//
// Created by zavier on 2021/7/18.
//

#include "Jobs.h"
#include <iostream>
bool Jobs::addJob(pid_t pid, int state, const std::string &cmd) {
    if(pid<=0)
        return false;
    for(int i=0;i<jobs.size();i++){
        if(jobs[i].pid==0){
            jobs[i].pid=pid;
            jobs[i].cmd=cmd;
            jobs[i].state=state;
            jobs[i].jid=nextJid++;
            if(nextJid>JOBSIZE)
                nextJid=1;
            currentJobs++;
            return true;
        }
    }
    std::cout<<"Too many Jobs"<<std::endl;
    return false;
}

bool Jobs::deleteByPid(pid_t pid) {
    for(auto &job:jobs){
        if(job.pid==pid){
            clearJob(job);
            return true;
        }
    }
    return false;
}

bool Jobs::deleteByJid(int jid) {
    for(auto &job:jobs){
        if(job.jid==jid){
            clearJob(job);
            return true;
        }
    }
    return false;
}

void Jobs::clearJob(Jobs::Job &job) {
    job.pid=0;
    job.jid=0;
    job.state=UNDEFINE;
    job.cmd.clear();
    currentJobs--;
    nextJid=maxJid()+1;
}

Jobs::Job *Jobs::getJobBypid(pid_t pid) {
    for(auto &job:jobs){
        if(job.pid==pid){
            return &job;
        }
    }
    return nullptr;
}

Jobs::Job *Jobs::getJobByJid(int jid) {
    for(auto &job:jobs){
        if(job.jid==jid){
            return &job;
        }
    }
    return nullptr;
}

void Jobs::printJobs() {
    if(currentJobs==0){
        return;
    }
    for(auto &job:jobs){
        if(job.pid==0) continue;
        std::cout<<"任务["<<job.jid<<"] pid:"<<job.pid<<" "<<job.cmd<<' ';
        switch (job.state) {
            case FOREGROUND:
                std::cout<<"前台运行 ";
                break;
            case BACKGROUND:
                std::cout<<"后台运行 ";
                break;
            case STOP:
                std::cout<<"停止运行 ";
                break;
            default:
                std::cout<<"错误 ";
        }
        std::cout<<std::endl;
    }
}

size_t Jobs::size() {
    return currentJobs;
}

std::ostream &operator<<(std::ostream &os, Jobs jobs) {
    jobs.printJobs();
    return os;
}

Jobs::Job *Jobs::getForegroundJob() {
    for(auto &job:jobs){
        if(job.state==FOREGROUND)
            return &job;
    }
    return nullptr;
}
pid_t Jobs::getForegroundJobPid() {
    auto job=getForegroundJob();
    if(job== nullptr)
        return 0;
    return job->pid;
}
int Jobs::maxJid() {
    int res=0;
    for(auto &job:jobs){
        if(job.jid>res){
            res=job.jid;
        }
    }
    return res;
}
