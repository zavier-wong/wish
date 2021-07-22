//
// Created by zavier on 2021/7/18.
//

#ifndef WISH_JOBS_H
#define WISH_JOBS_H
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
class Jobs {
public:
    enum JobState{
        UNDEFINE, //未定义
        BACKGROUND,//后台任务
        FOREGROUND, //前台任务
        STOP //暂停任务
    };
    struct Job{
        pid_t pid;              /* job PID */
        int jid;                /* job ID [1, 2, ...] */
        int state;              /* UNDEF, BG, FG, or ST */
        std::string cmd;  /* command line */
    };
    Jobs(){
        nextJid=1;
        jobs.resize(JOBSIZE);
        for(auto &job:jobs){
            clearJob(job);
        }

    }
    bool addJob(pid_t pid,int state,const std::string &cmd);
    bool deleteByPid(pid_t pid);
    bool deleteByJid(int jid);
    Job* getJobBypid(pid_t pid);
    Job* getJobByJid(int jid);
    Job* getForegroundJob();
    pid_t getForegroundJobPid();
    void printJobs();
    size_t size();
    int maxJid();
    friend std::ostream &operator<<(std::ostream &os,Jobs);


private:
    static constexpr int JOBSIZE=100;
    int nextJid;
    size_t currentJobs;
    std::vector<Job> jobs;

    void clearJob(Job &job);
};


#endif //WISH_JOBS_H
