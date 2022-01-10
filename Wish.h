//
// Created by zavier on 2021/7/16.
//

#ifndef WISH_WISH_H
#define WISH_WISH_H
#include <string>//
// Created by zavier on 2021/7/16.
//

#ifndef WISH_WISH_H
#define WISH_WISH_H
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include "Jobs.h"

extern Jobs jobs;

class Wish {
public:
    Wish(int argc,char *arg[]);
    Wish(const Wish& w) = delete;
    Wish& operator=(const Wish& w) = delete;

    void run();

private:
    static constexpr int LINESIZE=256;
    static constexpr int TOKENSIZE=50;
    static constexpr int ARGSIZE=50;

    std::string prompt;
    std::string home_path;
    std::string user_name;
    std::vector<std::string> PATH;
    std::string current_work_path;
    std::set<std::string > commands;
    std::vector<std::string> shell_argv;
    std::map<std::string ,std::vector<std::string>> alias;

    void welcome();
    std::string getCwd();
    void printPrompt();
    void eval(std::string line);
    void waitForeground(int pid);
    bool builtinCommand(const std::vector<std::string>& arg);
    int execCommand(const std::vector<std::string>& arg);
    std::vector<std::string> getCommandbyAlias(std::vector<std::string>& argv);
    void doPipe(std::vector<std::string> &commands);
    bool parseline(
            std::string line,
            std::vector<std::string>& argv,
            std::vector<std::pair<int,int>> &redirect,
            int &jobState
            );

    bool builtinAlias(const std::vector<std::string> &arg);
    bool builtinCd(const std::vector<std::string> &arg);
    bool builtinKill(const std::vector<std::string> &arg);
    bool builtinFg(const std::vector<std::string> &arg);
    bool builtinBg(const std::vector<std::string> &arg);
    bool builtinType(const std::vector<std::string> &arg);
    bool builtinEcho(const std::vector<std::string> &arg);

    static std::string& strip(std::string &str);
    static pid_t getPid(const std::string &str);
    static bool removeQuotationMarks(std::string &arg);
    static std::vector<std::string> split(const std::string &line,char sp);
    static std::string argToString(const std::vector<std::string> &arg);
    static bool searchDirectory(const std::string &path,const std::string &file);
    static bool parseRedirect(std::vector<std::string> &arg,std::vector<std::pair<int,int>> &redirect);

    static void handleSigchld(int signum);
    static void handleSigint(int signum);
    static void handleSigstop(int signum);
};


#endif //WISH_WISH_H

#include <vector>
#include <iostream>
#include <map>
#include <set>
#include "Jobs.h"

extern Jobs jobs;

class Wish {
public:
    Wish(int argc,char *arg[]);
    Wish(const Wish& w) = delete;
    Wish& operator=(const Wish& w) = delete;

    void run();

//private:
    static constexpr int LINESIZE=256;
    static constexpr int TOKENSIZE=50;
    static constexpr int ARGSIZE=50;

    std::string prompt;
    std::string home_path;
    std::string user_name;
    std::vector<std::string> PATH;
    std::string current_work_path;
    std::set<std::string > commands;
    std::vector<std::string> shell_argv;
    std::map<std::string ,std::vector<std::string>> alias;

    void __do__(std::string &cmd,pid_t pid);
    void welcome();
    std::string getCwd();
    void printPrompt();
    void eval(std::string line);
    void waitForeground(int pid);
    bool builtinCommand(const std::vector<std::string>& arg);
    int execCommand(const std::vector<std::string>& arg);
    std::vector<std::string> getCommandbyAlias(std::vector<std::string>& argv);
    void doPipe(std::vector<std::string> &commands);
    bool parseline(
            std::string line,
            std::vector<std::string>& argv,
            std::vector<std::pair<int,int>> &redirect,
            int &jobState
    );

    bool builtinAlias(const std::vector<std::string> &arg);
    bool builtinCd(const std::vector<std::string> &arg);
    bool builtinKill(const std::vector<std::string> &arg);
    bool builtinFg(const std::vector<std::string> &arg);
    bool builtinBg(const std::vector<std::string> &arg);
    bool builtinType(const std::vector<std::string> &arg);
    bool builtinEcho(const std::vector<std::string> &arg);
    bool builtinHelp(const std::vector<std::string> &arg);

    static std::string& strip(std::string &str);
    static pid_t getPid(const std::string &str);
    static bool removeQuotationMarks(std::string &arg);
    static std::vector<std::string> split(const std::string &line,char sp);
    static std::string argToString(const std::vector<std::string> &arg);
    static bool searchDirectory(const std::string &path,const std::string &file);
    static bool parseRedirect(std::vector<std::string> &arg,std::vector<std::pair<int,int>> &redirect);

    static void handleSigchld(int signum);
    static void handleSigint(int signum);
    static void handleSigstop(int signum);
};


#endif //WISH_WISH_H
