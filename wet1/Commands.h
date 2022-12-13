#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <map>
#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <thread>
#include <fstream>
#include <regex>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define NO_FOREGROUND (-1)
#define SYSCALL_FAIL (-1)

typedef unsigned int jid;

class Command {
 protected:
  std::vector<std::string> args;
  std::string line;
  std::string original_line;
  bool is_bg;
  jid job_id;
 public:
  Command(const char* cmd_line);
  virtual ~Command() = default;
  virtual void execute() = 0;
  std::string& getLine() { return this->line;}
  std::string& getOriginalLine() { return this->original_line;}
  // void setLine(std::string& line) { this->line = line;}
  void setOriginalLine(std::string& original_line) { this->original_line = original_line;}
  bool getIsBg() {return this->is_bg;}
  jid getJobId() {return this->job_id;}
  void setJobId(jid job_id) {this->job_id = job_id;}
  virtual void prepare(){}
  virtual void cleanup(){}
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  int pipe_arr[2];
  std::string first_cmd;
  std::string second_cmd;
  bool is_err;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
  std::string file_name;
  bool append;
  int stdout_copy;
  int file_fd;
  std::string cmd;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  void prepare() override;
  void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand { 
public:
  ChangePromptCommand(const char* cmd_line);
  virtual ~ChangePromptCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
  std::string& last_pwd;
public:
  ChangeDirCommand(const char* cmd_line, std::string& last_pwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
 JobsList& jobs;
public:
  QuitCommand(const char* cmd_line, JobsList& jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
    pid_t pid;
    time_t time_created;
    bool is_stopped;
    Command* cmd;
    public:
      JobEntry(Command* cmd, pid_t pid, bool is_stopped): is_stopped(is_stopped) {
        this->cmd = cmd;
        this->pid = pid;
        time_created = time(nullptr);
      }
      ~JobEntry() = default;
      pid_t getJobPid() {return pid;}
      pid_t getTimeCreated() {return time_created;}
      Command* getCommand() {return cmd;}
      bool getIsStopped() {return is_stopped;}
      void setStopped(bool x) {this->is_stopped = x;}
  };
 std::map<jid, JobEntry> job_map;
 
 public:
  JobsList();
  ~JobsList() = default;
  void addJob(Command* cmd, pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry& getJobById(jid jobId);
  void removeJobById(jid jobId);
  JobEntry& getLastJob(jid* lastJobId);
  JobEntry& getLastStoppedJob(jid *jobId);

  class JobIdMissing: public std::exception {};
  class EmptyList: public std::exception {};
  class NoStoppedJob: public std::exception {};
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 JobsList& jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList& jobs);
  virtual ~JobsCommand() = default;
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList& jobs;
 public:
  ForegroundCommand(const char* cmd_line, JobsList& jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList& jobs;
 public:
  BackgroundCommand(const char* cmd_line, JobsList& jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
  void evacuate(int fd, std::string& contents);
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
  JobsList& jobs;
 public:
  KillCommand(const char* cmd_line, JobsList& jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  std::string prompt;
  std::string last_pwd;
  Command* foreground_process;
  pid_t fg_pid;
  JobsList jobs_list;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  JobsList& getJobsList() {return this->jobs_list;}
  std::string& getPrompt() { return prompt; } 
  void setPrompt(std::string newPrompt) { prompt = newPrompt; }
  std::string& getLastPwd() { return last_pwd; }
  void setLastPwd(std::string newLastPwd) { last_pwd = newLastPwd; }
  Command* getForegroundProcess() { return foreground_process; }
  void setForegroundProcess(Command* new_foreground_process) { foreground_process = new_foreground_process; }
  pid_t getFgPid() { return fg_pid; }
  void setFgPid(pid_t fg_pid) { this->fg_pid = fg_pid; }
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
