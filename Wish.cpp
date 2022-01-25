//
// Created by zavier on 2021/7/16.
//

#include "Wish.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <pwd.h>
#include <time.h>
#include <cctype>
#include <dirent.h>
#include <fcntl.h>
Jobs jobs;

inline void error_msg(const std::string &cmd,const std::string &msg){
    std::cout<<"\033[0;31m";
    std::cout<<cmd<<": "<<msg<<std::endl;
    std::cout<<"\033[0m";
}
/**
 * 初始化shell环境
 * */
Wish::Wish(int argc, char **arg) {
    for(int i=0;i<argc;i++){
        shell_argv.emplace_back(arg[i]);
    }

    alias={
            {"ls",{"ls", "--color=auto"}},
            {"ll",{"ls","-alF"}},
            {"l",{"ls","-CF"}},
            {"rm",{"rm","-f"}}
    };
    struct passwd *pwd = getpwuid(getuid());
    user_name=pwd->pw_name;
    PATH= split(getenv("PATH"),':');
    prompt="$ ";
    home_path= getenv("HOME");
    current_work_path=getCwd();
    commands={"alias","bg","cd","echo","exit","fg","jobs","kill","pwd","quit","type"};
}
/**
 * shell运行
 * */
void Wish::run() {
    std::string line;

    dup2(STDOUT_FILENO,STDERR_FILENO);

    signal(SIGCHLD,handleSigchld);
    signal(SIGINT,handleSigint);
    signal(SIGTSTP,handleSigstop);
    welcome();

    while(true){
        printPrompt();
        if(getline(std::cin,line))
            eval(line);
        else{
            std::cout<<"\nBye "<<user_name<<", see you next time~"<<std::endl;
            exit(0);
        }
    }

}
/**
 * 处理
 * */
void Wish::eval(std::string line) {
    if(line.empty())
        return;
    int pid;
    int jobState=Jobs::FOREGROUND;
    sigset_t mask;
    std::vector<std::string> arg;
    //按;分割命令
    std::vector<std::string> commands = split(line,';');
    if(commands.empty()){
        error_msg("语法错误","无法解析的引号");
        return;
    }

    for(auto &fullCmd : commands){
        auto cmd=fullCmd;
        std::vector<std::string> pipeCommand = split(fullCmd,'|');
        if(pipeCommand.size()>1){
            doPipe(pipeCommand);
            continue;
        }

        std::vector<std::pair<int,int>> rd;
        if(!parseline(cmd,arg,rd,jobState))
            return;

        if(builtinCommand(arg))
            continue;
        pid = fork();
        if(pid<0){
            std::cout<<"fork error"<<std::endl;
            return;
        }
        if(pid==0) {
            if (sigemptyset(&mask) < 0) {
                std::cout << "sigemptyset error" << std::endl;
                return;
            }
            if (sigaddset(&mask, SIGCHLD)) {
                std::cout << "sigaddset error" << std::endl;
                return;
            }
            if (sigaddset(&mask, SIGINT)) {
                std::cout << "sigaddset error" << std::endl;
                return;
            }
            if (sigaddset(&mask, SIGTSTP)) {
                std::cout << "sigaddset error" << std::endl;
                return;
            }
            if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
                std::cout << "sigprocmask error" << std::endl;
                return;
            }
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            if (setpgid(getpid(), 0) < 0) {
                std::cout << "setpgid error" << std::endl;
                return;
            }

            for(auto i:rd){
                dup2(i.second,i.first);
            }
            if (execCommand(arg) < 0) {
                error_msg(arg[0], "Command not found");
                exit(0);
            }
        }
        jobs.addJob(pid,(jobState == Jobs::FOREGROUND ? Jobs::FOREGROUND : Jobs::BACKGROUND), fullCmd);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        if(jobState == Jobs::FOREGROUND){
            //等待前台任务结束
            waitForeground(pid);
        }else{
            std::cout<<"["<<jobs.getJobBypid(pid)->jid<<"] "<<pid<<" "<<arg[0]<<std::endl;
        }
    }
}
/**
 * 执行管道
 * */
void Wish::doPipe(std::vector<std::string> &pipeCommand) {
    int pid;
    int id = -1;
    int p[100][2] = {};
    int jobState = Jobs::FOREGROUND;
    sigset_t mask;
    std::vector<std::string> arg;

    //管道并行
    for(id = 0; id < pipeCommand.size(); id++){
        if (id != pipeCommand.size() - 1) {
            pipe(p[id]);
        }
        if((pid = fork()) < 0)
        {
            perror("fork error");
            exit(1);
        }
        if( pid > 0 ) {
            continue;
        }
        if(id == 0){
            dup2(p[0][1],STDOUT_FILENO);
            close(p[0][0]);
            close(p[0][1]);
        }else if(id == pipeCommand.size()-1){
            dup2(p[id - 1][0],STDIN_FILENO);
            for (int i = 0; i < id; i++) {
                close(p[i][0]);
                close(p[i][1]);
            }
        } else {
            dup2(p[id - 1][0], STDIN_FILENO);
            dup2(p[id][1], STDOUT_FILENO);

            for (int i = 0; i <= id; i++) {
                close(p[i][0]);
                close(p[i][1]);
            }
        }

        std::vector<std::pair<int,int>> rd;
        if(!parseline(pipeCommand[id],arg,rd,jobState))
            exit(0);

        if(builtinCommand(arg))
            return;
        if (sigemptyset(&mask) < 0) {
            std::cout << "sigemptyset error" << std::endl;
            return;
        }
        if (sigaddset(&mask, SIGCHLD)) {
            std::cout << "sigaddset error" << std::endl;
            return;
        }
        if (sigaddset(&mask, SIGINT)) {
            std::cout << "sigaddset error" << std::endl;
            return;
        }
        if (sigaddset(&mask, SIGTSTP)) {
            std::cout << "sigaddset error" << std::endl;
            return;
        }
        if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
            std::cout << "sigprocmask error" << std::endl;
            return;
        }
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        if (setpgid(getpid(), 0) < 0) {
            std::cout << "setpgid error" << std::endl;
            return;
        }

        if (execCommand(arg) < 0) {
            error_msg(arg[0], "Command not found");
            exit(0);
        }
    }
    for (int i = 0; i < pipeCommand.size() - 1; i++) {
        close(p[i][0]);
        close(p[i][1]);
    }
    jobs.addJob(pid,Jobs::FOREGROUND, argToString(pipeCommand));
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    waitForeground(pid);
}

/**
 * 解析命令,解析别名，提取参数并判断是否为后台命令
 * */
bool Wish::parseline(
        std::string line, std::vector<std::string> &argv,
        std::vector<std::pair<int,int>> &redirect,int &jobState) {

    //line= strip(line);
    argv= split(line,' ');
    bool res = parseRedirect(argv,redirect);
    if(!res){
        return false;
    }
    getCommandbyAlias(argv);

    for(auto &s:argv) {
        //去除参数引号
        removeQuotationMarks(s);
    }

    if(argv.back()=="&") {
        argv.pop_back();
        jobState = Jobs::BACKGROUND;
    }
    else jobState = Jobs::FOREGROUND;
    return true;
}

/**
 * 如果是内建命令就执行，否则返回false；
 * 内建命令：alias，bg，cd，echo，exit，fg，jobs，kill，pwd,quit,type
 * */
bool Wish::builtinCommand(const std::vector<std::string>& arg) {
    const std::string &cmd=arg[0];
    if(cmd=="alias"){
        builtinAlias(arg);
        return true;
    } else if(cmd=="bg"){
        builtinBg(arg);
        return true;
    } else if(cmd=="cd"){
        return builtinCd(arg);
    } else if(cmd=="echo"){
        builtinEcho(arg);
        return true;
    } else if(cmd=="exit"||cmd=="quit"){
        std::cout<<"Bye bye~"<<std::endl;
        exit(0);
    } else if(cmd=="fg"){
        builtinFg(arg);
        return true;
    } else if(cmd=="history"){
        //todo
        return true;
    } else if(cmd=="jobs"){
        std::cout<<jobs;
        return true;
    } else if(cmd=="kill"){
        builtinKill(arg);
        return true;
    } else if(cmd=="pwd"){
        std::cout<<current_work_path<<std::endl;
        return true;
    } else if(cmd=="type"){
        builtinType(arg);
        return true;
    }

    return false;
}

/**
 * 运行命令
 * */
int Wish::execCommand(const std::vector<std::string> &arg) {
    extern char** environ;
    if(arg.size()==0) return -1;
    char * argv[ARGSIZE];

    for (int i = 0; i < arg.size(); ++i) {
        argv[i]=const_cast<char *>(arg[i].c_str());
    }

    argv[arg.size()]=0;
    return execvpe(arg[0].c_str(),argv,environ);
}

void Wish::waitForeground(int pid) {
    if(pid==0) return;
    sigset_t s;
    sigemptyset(&s);

    while (pid == jobs.getForegroundJobPid()){
        sigsuspend(&s);
    }
}

void Wish::handleSigchld(int signum) {
    int pid;
    int stat;
    sigset_t mask,save;
    sigfillset(&mask);
    while((pid=waitpid(-1,&stat,WNOHANG|WUNTRACED))>0){
        sigprocmask(SIG_BLOCK,&mask,&save);
        Jobs::Job *job = jobs.getJobBypid(pid);

        if(WIFSTOPPED(stat)){
            job->state=Jobs::STOP;
            std::cout<<"任务["<<job->jid<<"] pid:"<<job->pid<<" "<<job->cmd<<" 被信号 "<<WSTOPSIG(stat)<<" 停止"<<std::endl;
        } else {
            if(WIFSIGNALED(stat))
                std::cout<<"任务["<<job->jid<<"] pid:"<<job->pid<<" "<<job->cmd<<" 被信号 "<<WTERMSIG(stat)<<" 终止"<<std::endl;
            jobs.deleteByPid(pid);
        }
        sigprocmask(SIG_SETMASK,&save,NULL);
    }

}

void Wish::handleSigint(int signum) {
    sigset_t mask,save;
    sigfillset(&mask);

    sigprocmask(SIG_BLOCK,&mask,&save);
    auto job=jobs.getForegroundJob();
    sigprocmask(SIG_SETMASK,&save,NULL);
    if(job == nullptr)
        return;
    //std::cout<<job->pid<<std::endl;
    kill(-job->pid,SIGINT);
}

void Wish::handleSigstop(int signum) {
    sigset_t mask,save;
    sigfillset(&mask);

    sigprocmask(SIG_BLOCK,&mask,&save);
    auto job=jobs.getForegroundJob();
    sigprocmask(SIG_SETMASK,&save,NULL);
    if(job == nullptr)
        return;
    //std::cout<<job->pid<<std::endl;
    kill(-job->pid,SIGTSTP);
}

/**
 * 内建命令cd
 * */
bool Wish::builtinCd(const std::vector<std::string> &arg) {
    const char * dir= nullptr;
    if(arg.size()==1){
        dir=home_path.c_str();
    }else if(arg.size()==2){
        if(arg[1]=="~")
            dir=home_path.c_str();
        else
            dir=arg[1].c_str();
    } else{
        error_msg("cd","Too many argument");
        return true;
    }
    if(chdir(dir)<0){
        error_msg("cd","No such file or directory");
        return true;
    }
    current_work_path=getCwd();
    return true;
}


/**
 * 内建命令kill
 * */
bool Wish::builtinKill(const std::vector<std::string> &arg) {
    bool sigFlag= false;
    //int killSig=SIGTERM;
    int killSig=SIGKILL;
    if(arg.size()==1){
        std::cout<<"kill：用法： kill [-n 信号编号] 进程号 | 任务号 ... 或 kill -l [信号声明]"<<std::endl;
        return false;
    }
    if(arg[1]=="-l"){
        std::cout<<" 1) SIGHUP\t 2) SIGINT\t 3) SIGQUIT\t 4) SIGILL\t 5) SIGTRAP\n"
                   " 6) SIGABRT\t 7) SIGBUS\t 8) SIGFPE\t 9) SIGKILL\t10) SIGUSR1\n"
                   "11) SIGSEGV\t12) SIGUSR2\t13) SIGPIPE\t14) SIGALRM\t15) SIGTERM\n"
                   "16) SIGSTKFLT\t17) SIGCHLD\t18) SIGCONT\t19) SIGSTOP\t20) SIGTSTP\n"
                   "21) SIGTTIN\t22) SIGTTOU\t23) SIGURG\t24) SIGXCPU\t25) SIGXFSZ\n"
                   "26) SIGVTALRM\t27) SIGPROF\t28) SIGWINCH\t29) SIGIO\t30) SIGPWR\n"
                   "31) SIGSYS\t34) SIGRTMIN\t35) SIGRTMIN+1\t36) SIGRTMIN+2\t37) SIGRTMIN+3\n"
                   "38) SIGRTMIN+4\t39) SIGRTMIN+5\t40) SIGRTMIN+6\t41) SIGRTMIN+7\t42) SIGRTMIN+8\n"
                   "43) SIGRTMIN+9\t44) SIGRTMIN+10\t45) SIGRTMIN+11\t46) SIGRTMIN+12\t47) SIGRTMIN+13\n"
                   "48) SIGRTMIN+14\t49) SIGRTMIN+15\t50) SIGRTMAX-14\t51) SIGRTMAX-13\t52) SIGRTMAX-12\n"
                   "53) SIGRTMAX-11\t54) SIGRTMAX-10\t55) SIGRTMAX-9\t56) SIGRTMAX-8\t57) SIGRTMAX-7\n"
                   "58) SIGRTMAX-6\t59) SIGRTMAX-5\t60) SIGRTMAX-4\t61) SIGRTMAX-3\t62) SIGRTMAX-2\n"
                   "63) SIGRTMAX-1\t64) SIGRTMAX"<<std::endl;
        return false;
    }
    if(arg[1][0]=='-'){
        sigFlag= true;
        for(int i=1;i<arg[1].size();i++){
            if(!isdigit(arg[1][i])){
                std::string emsg=arg[1]+":无效的信号声明";
                error_msg("kill",emsg);
            }
        }
        killSig=std::stoi(arg[1].substr(1));
    }

    for (int i = 1; i < arg.size(); ++i) {
        if(i==1 && sigFlag)
            continue;
        pid_t pid= getPid(arg[i]);
        if(pid==0){
            std::string emsg=arg[i]+":无此任务";
            error_msg("kill",emsg);
            continue;
        }
        kill(-pid,killSig);
    }
    return true;
}
/**
 * 内建命令fg
 * */
bool Wish::builtinFg(const std::vector<std::string> &arg) {
    pid_t pid;
    if(arg.size()==1){
        error_msg("fg","current: no such job");
        return false;
    }
    for (int i = 1; i < arg.size(); ++i) {
        pid= getPid(arg[i]);
        if(pid==0){
            error_msg("fg",arg[i]+": no such job");
            continue;
        }
        auto job=jobs.getJobBypid(pid);
        if(job->state==Jobs::FOREGROUND){
            error_msg("fg","job already in foreground");
        }else{
            kill(-pid,SIGCONT);
            job->state=Jobs::FOREGROUND;
            waitForeground(pid);
        }
    }
    return true;
}
/**
 * 内建命令bg
 * */
bool Wish::builtinBg(const std::vector<std::string> &arg) {
    pid_t pid;
    if(arg.size()==1){
        error_msg("bg","current: no such job");
        return false;
    }
    for (int i = 1; i < arg.size(); ++i) {
        pid= getPid(arg[i]);
        if(pid==0){
            error_msg("bg",arg[i]+": no such job");
            continue;
        }
        auto job=jobs.getJobBypid(pid);
        if(job->state==Jobs::BACKGROUND){
            error_msg("bg","job already in background");
        }else{
            kill(-pid,SIGCONT);
            job->state=Jobs::BACKGROUND;
        }
    }
    return true;
}
/**
 * 内建命令echo
 * */
bool Wish::builtinEcho(const std::vector<std::string> &arg) {
    for (int i = 1; i < arg.size(); ++i) {
        std::string tmp=arg[i];
        removeQuotationMarks(tmp);
        std::cout<<tmp<<' ';
    }
    std::cout << std::endl;
    return true;
}
/**
 * 内建命令type
 * */
bool Wish::builtinType(const std::vector<std::string> &arg) {
    if(arg.size()==1) return true;

    for(int i=1;i<arg.size();i++){
        if(alias.find(arg[i])!=alias.end()){
            std::cout<<arg[i]<<" 是 \""<<argToString(alias[arg[i]])<<"\" 的别名"<<std::endl;
        }else if(commands.find(arg[i])!=commands.end()){
            std::cout<<arg[i]<<" 是 shell 内建命令"<<std::endl;
        } else{
            for(auto &dir:PATH){
                if(searchDirectory(dir,arg[1])){
                    std::cout<<arg[i]<<" 是 "<<dir<<std::endl;
                    return true;
                }
            }
            error_msg("type",arg[1]+":未找到");
        }
    }
    return true;
}
/**
 * 内建命令alias
 * */
bool Wish::builtinAlias(const std::vector<std::string> &arg) {
    if(arg.size()==1){
        for(auto i:alias){
            std::cout<<"alias "<<i.first<<"=\""<<argToString(i.second)<<"\""<<std::endl;
        }
        return true;
    }
    for (int i = 1; i < arg.size(); ++i) {
        auto idx = arg[i].find_first_of('=');
        if(idx == std::string::npos){
            if(alias.find(arg[i]) != alias.end()){
                std::cout<<"alias "<<arg[i]<<"=\""<<argToString(alias[arg[i]])<<"\""<<std::endl;
            }else{
                error_msg("alias",arg[i]+":未找到");
            }
        }else{
            std::vector<std::string> as;
            auto cmd = arg[i].substr(0,idx);
            auto tmp = arg[i].substr(idx+1);

            if(tmp[0]=='\'' || tmp[0]=='\"'){
                if (tmp.size() == 1) {
                    alias.erase(cmd);
                    continue;
                }
                as = split(tmp.substr(1),' ');
            }else{
                as.push_back(tmp);
            }
            alias[cmd]=as;
        }
    }
    return true;
}
/**
 * 将参数列表转换为字符串
 * */
std::string Wish::argToString(const std::vector<std::string> &arg) {
    std::string res;
    for (int i = 0; i < arg.size(); ++i) {
        if(i) res.append(" ");
        res.append(arg[i]);
    }
    return res;
}
/**
 * 搜索文件是否在指定目录
 * */
bool Wish::searchDirectory(const std::string &path, const std::string &file) {
    struct dirent   *dp;
    char  name[101] = {0};
    DIR  *dfd;
    if( (dfd = opendir(path.c_str())) == NULL ){
        closedir(dfd);
        return false;
    }
    for(dp = readdir(dfd); NULL!=dp; dp = readdir(dfd))
    {
        if(dp->d_name==file){
            closedir(dfd);
            return true;
        }
    }
    closedir(dfd);
    return false;
}

/**
 * 获取当前工作路径
 * */
std::string Wish::getCwd() {
    std::string res;
    char str[LINESIZE];
    getcwd(str,LINESIZE);
    res=str;
    return res;
}
/**
 * 找出别名的最终命令
 * */
std::vector<std::string> Wish::getCommandbyAlias(std::vector<std::string>& argv) {
    std::string cmd=argv[0];
    std::vector<std::string> res;
    if(alias.find(cmd)==alias.end()){
        res.push_back(cmd);
        return res;
    }
    res=alias[cmd];

    if(alias.find(res[0])==alias.end()){
        return res;
    }
    auto t=alias[res[0]];

    for (int i = 1; i < res.size(); ++i) {
        t.push_back(res[i]);
    }
    argv.erase(argv.begin());
    argv.insert(argv.begin(),t.begin(),t.end());
    return t;
}

/**
 * 去除字符串头尾的空格
 * */
std::string &Wish::strip(std::string &str) {
    if(str.empty()){
        return str;
    }
    str.erase(0,str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ")+1);
    return str;
}
/**
 * 用sp对字符串进行切割,保留引号里内容
 * */
std::vector<std::string> Wish::split(const std::string &line,const char sp) {
    std::vector<std::string> res;
    std::string tmp;
    bool trapSingle= false;
    bool trapDouble= false;
    for(int i=0;i< line.size();i++){
        if(line[i]=='\''&&!trapDouble){
            if(!trapSingle){
                trapSingle= true;
            } else{
                trapSingle= false;
            }
            tmp.push_back(line[i]);
            continue;
        }else if(line[i]=='\"'&&!trapSingle){
            if(!trapDouble){
                trapDouble= true;
            } else{
                trapDouble= false;
            }
            tmp.push_back(line[i]);
            continue;
        }
        if(trapSingle || trapDouble || line[i] != sp){
            tmp.push_back(line[i]);
        } else if(tmp.size()){
            res.push_back(tmp);
            tmp.clear();
        }
    }
    if(tmp.size()) res.push_back(tmp);
    if(trapDouble||trapSingle)
        return std::vector<std::string>();
    return res;
}

pid_t Wish::getPid(const std::string &str) {
    bool isJid= false;
    std::string tmp;
    if(str[0]=='%'){
        isJid= true;
        tmp=str.substr(1);
    } else{
        tmp=str;
    }
    for(auto c:tmp){
        if(!isdigit(c))
            return 0;
    }
    int d=std::stoi(tmp);
    if(!isJid)
        return d;
    auto job=jobs.getJobByJid(d);
    if(job== nullptr)
        return 0;
    else
        return job->pid;
}
/**
 * shell开始界面
 * */
void Wish::welcome(){
    std::string st="\033[0;36m";
    std::string ed="\033[0m";
    std::string str=R"(            _         _
__      __ (_)  ___  | |__
\ \ /\ / / | | / __| | '_ \
 \ V  V /  | | \__ \ | | | |
  \_/\_/   |_| |___/ |_| |_|
Wish is still a simple shell interpreter , not yet for scripting.
The basic functions include command line, pipe, terminal reporting, I/O redirection and so on.
内建命令：alias，bg，cd，echo，exit，fg，jobs，kill，pwd,quit,type
外部命令：$PATH 下目录的所有命令
退出wish：Ctrl+D, exit , quit
)";
    std::cout<<st<<str<<ed;
}
/**
 * 打印提示符
 * */
void Wish::printPrompt() {
    time_t now;
    struct tm *tm_now;
    time(&now);
    tm_now = localtime(&now);
    std::cout<<"\033[0;32m";
    std::cout<<1900+tm_now->tm_year<<'-'<<1+tm_now->tm_mon<<'-'<< tm_now->tm_mday<<' '<<tm_now->tm_hour<<':'<<tm_now->tm_min<<':'<< tm_now->tm_sec<<std::endl;
    std::cout<<"\033[0m";
    std::cout<<"\033[0;31m";
    std::cout<<user_name<<" ^_^ ";
    std::cout<<"\033[0m";
    std::cout<<"\033[0;33m";
    std::cout<<current_work_path<<"\033[0m";
    std::cout<<prompt<<std::flush;
}
/**
 * 去除参数引号
 * */
bool Wish::removeQuotationMarks(std::string &arg) {
    auto idx=arg.find_first_of("'\"");
    if(idx==std::string::npos)
        return false;
    if(arg[0]=='\''||arg[0]=='\"')
        arg.erase(0,1);
    if(arg.back()=='\''||arg.back()=='\"')
        arg.pop_back();
    return true;
}
// > 1&>2 <  &>1
bool Wish::parseRedirect(std::vector<std::string> &arg, std::vector<std::pair<int, int>> &redirect) {
    int oldfd=-1;
    bool isFile=false;
    bool isAppend=false;
    bool isRead=false;
    std::vector<int> readyErase(arg.size(),0);
    std::vector<std::string> backup;
    auto isDigit=[](const std::string &s){
        return s.find_first_not_of("0123456789")==std::string::npos;
    };
    for (int i = 1; i < arg.size(); ++i) {
        if((arg[i]=="<"||arg[i]==">"||arg[i]==">>")&&(i+1==arg.size())){
            error_msg("语法错误","无法解析重定向");
            return false;
        }
        if(arg[i]=="<"){
            isFile= true;
            isRead= true;
            oldfd=STDIN_FILENO;
        } else if(arg[i]==">"){
            isRead= false;
            isFile= true;
            oldfd=STDOUT_FILENO;
        } else if(arg[i]==">>"){
            isRead= false;
            isAppend=true;
            isFile= true;
            oldfd=STDOUT_FILENO;
        } else if(arg[i]==">&"){
            if(i-1>0&&isDigit(arg[i-1])){
                oldfd= std::stoi(arg[i]);
                readyErase[i-1]=1;
            }else{
                oldfd=STDOUT_FILENO;
            }
            isFile= false;
        } else if(arg[i]=="<&"){
            if(i-1>0&&isDigit(arg[i-1])){
                oldfd= std::stoi(arg[i]);
                readyErase[i-1]=1;
            }else{
                oldfd=STDIN_FILENO;
            }
            isFile= false;
        }
        if(oldfd>=0){
            int fd;
            if(!isFile){
                if(isDigit(arg[i+1])){
                    fd= std::stoi(arg[i+1]);
                }else {
                    fd=-1;
                }
            }else if(isFile){
                if(isRead){
                    fd= open(arg[i+1].c_str(),O_RDONLY);
                } else if(isAppend){
                    fd= open(arg[i+1].c_str(),O_CREAT | O_RDWR | O_APPEND,0666);
                } else{
                    fd= open(arg[i+1].c_str(),O_CREAT | O_RDWR | O_TRUNC,0666);
                }
                if(fd<0){
                    perror(arg[i+1].c_str());
                    return false;
                }

            }
            redirect.emplace_back(oldfd,fd);
            readyErase[i]=1;
            readyErase[i+1]=1;
            oldfd=-1;
        }
    }
    for (int i = 0; i < arg.size(); ++i) {
        if(readyErase[i])
            continue;
        backup.push_back(arg[i]);
    }
    arg=backup;
    return true;
}
