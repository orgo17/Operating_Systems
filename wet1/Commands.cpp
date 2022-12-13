#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

void _parseCommandLine(const char* cmd_line, std::vector<std::string>& args) {
  FUNC_ENTRY()
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args.push_back(s);
  }

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(string& cmd_line) {
  // find last character other than spaces
  unsigned int idx = cmd_line.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line.erase(cmd_line.find_last_not_of(WHITESPACE, idx)+1,1);
}

// TODO: Add your implementation for classes in Commands.h 
Command::Command(const char* cmd_line): line(cmd_line), original_line(cmd_line), is_bg(false) , job_id(0) {
  if(_isBackgroundComamnd(cmd_line)) {
    is_bg = true;
  }
  _removeBackgroundSign(line);
  _parseCommandLine(line.c_str(), this->args);
}

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ChangePromptCommand::execute() {
  if (this->args.size() > 1) {
    SmallShell::getInstance().setPrompt(this->args[1]);
  }
  else {
    SmallShell::getInstance().setPrompt("smash");
  }
}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
  char cwd[COMMAND_ARGS_MAX_LENGTH];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    cout << cwd << endl;
  }
  else {
    perror("smash error: getcwd failed");
  }
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string& last_pwd) : BuiltInCommand(cmd_line), last_pwd(last_pwd){}

void ChangeDirCommand::execute () {
  if(this->args.size() > 2){
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }
  if(this->args.size() == 1){
    std::cerr << "smash error:> \"" << this->getOriginalLine() << "\"" << std::endl;
    return;
  }
  char cwd[PATH_MAX+1];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("smash error: getcwd failed");
    return;
  }
  int res = SYSCALL_FAIL;
  // if we get input - move to last dir
  if ((this->args[1]).compare("-") == 0) {
    if((this->last_pwd).compare("") == 0){
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    res = chdir((this->last_pwd).c_str());
  }
  else{
    res = chdir(args[1].c_str());
  }
  if(res == SYSCALL_FAIL){
    perror("smash error: chdir failed");
    return;
  }
  last_pwd = std::string(cwd);
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute(){
  pid_t pid = fork();
  if(pid == 0) {
    if(setpgrp() == SYSCALL_FAIL){
      perror("smash error: setpgrp failed");
      return;
    }
    // simple external
    if(line.find('*') == std::string::npos && line.find('?') == std::string::npos) {
      std::vector<const char*> command_args;
      for(std::string& arg : this->args) {
        command_args.push_back(arg.c_str());
      }
      command_args.push_back(nullptr);
      
      execvp(command_args[0], const_cast<char* const*>(command_args.data()));
      perror("smash error: execvp failed");
      exit(EXIT_FAILURE);
    }
    // Complex external
    else {
      char arr[line.size()+1];
      strcpy(arr, line.c_str());
      char file[] = "/bin/bash";
      char c[] = "-c";
      execl("/bin/bash", file, c, arr, nullptr);
      perror("smash error: execl failed");
      exit(EXIT_FAILURE);
    }
  }
  else if(pid == SYSCALL_FAIL){
    perror("smash error: fork failed");
  }
  else {
    if(this->is_bg){
      SmallShell::getInstance().getJobsList().addJob(this, pid);
    }
    else{
      SmallShell::getInstance().setForegroundProcess(this);
      SmallShell::getInstance().setFgPid(pid);
      if(waitpid(pid, nullptr, WUNTRACED) == SYSCALL_FAIL){
        perror("smash error: waitpid failed");
      }
      SmallShell::getInstance().setFgPid(NO_FOREGROUND);
      SmallShell::getInstance().setForegroundProcess(nullptr);
    }
    
  }
}

JobsCommand::JobsCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute() {
  this->jobs.printJobsList();
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void ForegroundCommand::execute() {
  if(this->args.size() > 2){
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
  try{
    // Initalize 
    jid job_id = 0; 
    JobsList::JobEntry job(nullptr, 0, 0);
    if(this->args.size() == 1){
      job = jobs.getLastJob(&job_id);
    }
    else{
      job_id = std::stoi(this->args[1]); 
      job = jobs.getJobById(job_id);
    }
    pid_t pid = job.getJobPid();
    if(kill(pid, SIGCONT) == SYSCALL_FAIL){
      perror("smash error: kill failed");
      return;
    }
    this->job_id = job_id;
    jobs.removeJobById(job_id); // remove fg process from job list
    SmallShell::getInstance().setForegroundProcess(this); // update small shell fg fields
    SmallShell::getInstance().setFgPid(pid);
    std::cout << job.getCommand()->getOriginalLine() << " : " << job.getJobPid() << endl; // print command
    this->setOriginalLine(job.getCommand()->getOriginalLine());
    if(job.getCommand()->getIsBg()){
      this->line.push_back('&');
    }
    if(waitpid(pid, nullptr, WUNTRACED) == SYSCALL_FAIL){ // brings pid process back to fg
      perror("smash error: waitpid failed");
    }
    SmallShell::getInstance().setFgPid(NO_FOREGROUND); // update small shell fg fields
    SmallShell::getInstance().setForegroundProcess(nullptr);
  }
  catch(std::invalid_argument& e) {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
  }
  catch(JobsList::EmptyList& e){
    std::cerr << "smash error: fg: jobs list is empty" << std::endl;
  }
  catch(JobsList::JobIdMissing& e) {
    std::cerr << "smash error: fg: job-id " << std::stoi(this->args[1]) << " does not exist" << std::endl;
  }
}

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void BackgroundCommand::execute() {
  if(this->args.size() > 2){
    std::cerr << "smash error: bg: invalid arguments" << std::endl;
    return;
  }
  try{
    // Initalize 
    jid job_id = 0; 
    JobsList::JobEntry job(nullptr, 0, 0);
    if(this->args.size() == 1){
      job = jobs.getLastStoppedJob(&job_id);
    }
    else{
      job_id = std::stoi(this->args[1]); 
      job = jobs.getJobById(job_id);
    }
    if(!job.getIsStopped()){
      std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << std::endl;
      return;
    }
    pid_t pid = job.getJobPid();
    std::cout << job.getCommand()->getOriginalLine() << " : " << job.getJobPid() << endl; // print command 
    jobs.getJobById(job_id).setStopped(false);  // mark command as running
    if(kill(pid, SIGCONT) == SYSCALL_FAIL){
      perror("smash error: kill failed");
    }
  }
  catch(std::invalid_argument& e) {
    std::cerr << "smash error: bg: invalid arguments" << std::endl;
  }
  catch(const JobsList::NoStoppedJob& e){
    std::cerr << "smash error: bg: there is no stopped jobs to resume" << std::endl;
  }
  catch(const JobsList::JobIdMissing& e) {
    std::cerr << "smash error: bg: job-id " << std::stoi(this->args[1]) << " does not exist" << std::endl;
  }
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void QuitCommand::execute() {
  if(this->args.size() > 1 && (this->args[1]).compare("kill") == 0) {
    jobs.killAllJobs();
  }
  exit(0);
}

KillCommand::KillCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void KillCommand::execute() {
  if(this->args.size() != 3) {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  try {
    JobsList::JobEntry& job = jobs.getJobById(std::stoi(this->args[2]));
    if(this->args[1][0] != '-'){
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      return;
    }
    std::string kill_str = this->args[1].substr(1);
    int kill_num = std::stoi(kill_str);
    if(kill(job.getJobPid(), kill_num) == -1) {
      perror("smash error: kill failed");
      return;
    }
    if(kill_num == SIGTSTP) {
      job.setStopped(true);
    }
    else if(kill_num == SIGCONT) {
      job.setStopped(false);
    }
    std::cout << "signal number " << kill_num << " was sent to pid " << job.getJobPid() << std::endl;
  }
  catch(std::invalid_argument& e) {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
  }
  catch(JobsList::JobIdMissing& e) {
    std::cerr << "smash error: kill: job-id " << std::stoi(this->args[2]) << " does not exist" << std::endl;
  }
}

SetcoreCommand::SetcoreCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void SetcoreCommand::execute() {
  if(this->args.size() != 3) { // check for valid arguments
      std::cerr << "smash error: setcore: invalid arguments" << std::endl;
      return;
  }
  try{
    JobsList::JobEntry& job = SmallShell::getInstance().getJobsList().getJobById(std::stoi(this->args[1]));
    const unsigned int core = std::stoi(this->args[2]);
    const auto processor_count = std::thread::hardware_concurrency(); // find number of cpu's
    if(core < 0 || core >= processor_count) {
      std::cerr << "smash error: setcore: invalid core number" << std::endl;
      return;
    }
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    if(sched_setaffinity(job.getJobPid(), sizeof(set), &set) == -1) {
      perror("smash error: sched_setaffinity failed");
    }
  }
  catch(std::invalid_argument& e) {
    std::cerr << "smash error: setcore: invalid arguments" << std::endl;
  }
  catch(const JobsList::JobIdMissing& e) {
    std::cerr << "smash error: setcore: job-id " << std::stoi(this->args[1]) << " does not exist" << std::endl;
  }
}

static int countSubstring(std::string& substring, std::string& contents) {
  int occurrences = 0;
   std::string::size_type pos = 0;
   while ((pos = contents.find(substring, pos )) != std::string::npos) {
          ++ occurrences;
          pos += substring.length();
   }
   return occurrences;
}

FareCommand::FareCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void FareCommand::evacuate(int fd, std::string& contents) {
  close(fd);
  fd = open(this->args[1].c_str(), O_RDWR|O_TRUNC);
  write(fd, contents.c_str(), contents.size());
  close(fd);
}

void FareCommand::execute() {
  if(this->args.size() != 4) { // check for valid arguments
      std::cerr << "smash error: fare: invalid arguments" << std::endl;
      return;
  }
  std::fstream file(this->args[1], std::fstream::out | std::fstream::in);
  if(!file){
    perror("smash error: open failed");
    return;
  }
  std::stringstream strStream;
  try { strStream << file.rdbuf(); }
  catch(...) {
    perror("smash error: read failed");
    return;
  }
  try { file.close(); }
  catch(...) {
    perror("smash error: close failed");
    return;
  }
  std::string contents = strStream.str();
  int counter = countSubstring(this->args[2], contents);
  std::string updated_contents = std::regex_replace(contents, std::regex(this->args[2]), this->args[3]);
  int fd = open(this->args[1].c_str(), O_RDWR|O_TRUNC);
  if(fd == SYSCALL_FAIL) {
    perror("smash error: open failed");
    return;
  }
  struct rlimit lim;
  if(getrlimit(RLIMIT_FSIZE, &lim) == -1){
    perror("smash error: getrlimit failed");
    this->evacuate(fd, contents);
    return;
  }
  if(lim.rlim_cur < updated_contents.size()) {
    this->evacuate(fd, contents);
    return;
  }
  ssize_t res_write = write(fd, updated_contents.c_str(), updated_contents.size());
  while(res_write < (long)updated_contents.size()) {
    if (res_write == SYSCALL_FAIL){
      perror("smash error: write failed");
      this->evacuate(fd, contents);
      return;
    }
    close(fd);
    fd = open(this->args[1].c_str(), O_RDWR|O_TRUNC);
    res_write = write(fd, updated_contents.c_str(), updated_contents.size());
  }
  if(close(fd) == SYSCALL_FAIL) {
    perror("smash error: close failed");
    return;
  }
  std::cout << "replaced " << counter << " instances of the string \"" << this->args[2] << "\"" << std::endl;
}

////////////////////////////////************************** Redirection implementation

RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line), append(false), stdout_copy(STDOUT_FILENO), file_fd(-1), cmd(""){
  std::size_t seperator = line.find_first_of(">");
  std::size_t second = seperator + 1;
  if(line.find(">>") != std::string::npos) {
    this->append = true;
    second += 1;
  }
  this->cmd = line.substr(0, seperator);
  this->file_name = _trim(line.substr(second));
  
  this->prepare();
}

void RedirectionCommand::prepare() {
  this->stdout_copy = dup(STDOUT_FILENO);
  if(this->stdout_copy == SYSCALL_FAIL){
    perror("smash error: dup failed");
    return;
  }
  if(close(STDOUT_FILENO) == SYSCALL_FAIL) {
    perror("smash error: close failed");
    return;
  }
  if(this->append){
    this->file_fd = open((this->file_name).c_str(), O_RDWR|O_APPEND|O_CREAT, 0655);
  }
  else {
    this->file_fd = open((this->file_name).c_str(), O_RDWR|O_CREAT|O_TRUNC, 0655);
  }
  if(this->file_fd == SYSCALL_FAIL) {
    perror("smash error: open failed");
  }
}

void RedirectionCommand::execute() {
  if(this->file_fd != SYSCALL_FAIL && this->stdout_copy != SYSCALL_FAIL) {
    SmallShell::getInstance().executeCommand(this->cmd.c_str());
  }
  cleanup();
}

void RedirectionCommand::cleanup() {
  if((this->file_fd) != SYSCALL_FAIL) {
    if(close(this->file_fd) == SYSCALL_FAIL){
          perror("smash error: close failed");
          return;
        }
  }
  if(dup2(this->stdout_copy, STDOUT_FILENO) == SYSCALL_FAIL) {
    perror("smash error: dup2 failed");
    return;
  }
  if(close(this->stdout_copy) == SYSCALL_FAIL){
    perror("smash error: close failed");
    return;
  }
}

////////////////////////////////************************** Pipe implementation

PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line), is_err(false) {
  std::size_t pipe_index = original_line.find_first_of("|");
  if(this->original_line.size() < (pipe_index + 2)) {       //check valid input
    std::cerr << "smash error:> \"" << this->getOriginalLine() << "\"";
    return;
  }
  this->first_cmd = original_line.substr(0, pipe_index);
  if(this->original_line[pipe_index+1] == '&') {    // check for |& pipe
    this->is_err = true;
    this->second_cmd = original_line.substr(pipe_index+2);
  }
  else {
    this->second_cmd = original_line.substr(pipe_index+1);
  }
  if(pipe(this->pipe_arr) == SYSCALL_FAIL) {
    perror("smash error: pipe failed");
  }
}

void PipeCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  int output = 1;
  pid_t first_cmd_child = fork(); // first child
  if(first_cmd_child == 0) {
    if(setpgrp() == SYSCALL_FAIL){
      perror("smash error: setpgrp failed");
      return;
    }
    if(is_err) output = 2;
    if(dup2(this->pipe_arr[1], output) == SYSCALL_FAIL) {
      perror("smash error: dup2 failed");
    }
    if(close(this->pipe_arr[0]) == SYSCALL_FAIL) {
      perror("smash error: close failed");
    }
    if(close(this->pipe_arr[1]) == SYSCALL_FAIL) {
      perror("smash error: close failed");
    }
    smash.executeCommand(this->first_cmd.c_str());
    exit(0);
  }
  else if(first_cmd_child == SYSCALL_FAIL) {
    perror("smash error: fork failed");
    return;
  }
  else {
    // second child
    pid_t second_cmd_child = fork();
    if(second_cmd_child == 0) {  
      if(setpgrp() == SYSCALL_FAIL){
        perror("smash error: setpgrp failed");
        return;
      }
      if(dup2(this->pipe_arr[0], STDIN_FILENO) == SYSCALL_FAIL) {
        perror("smash error: dup2 failed");
      }
      if(close(this->pipe_arr[0]) == SYSCALL_FAIL) {
        perror("smash error: close failed");
      }
      if(close(this->pipe_arr[1]) == SYSCALL_FAIL) {
        perror("smash error: close failed");
      }
      smash.executeCommand(this->second_cmd.c_str());
      exit(0);
    }
    else if(second_cmd_child == SYSCALL_FAIL) {
      perror("smash error: fork failed");
    }
    else {
      // close father fd for pipe
      if(close(this->pipe_arr[0]) == SYSCALL_FAIL) {
        perror("smash error: close failed");
      }
      if(close(this->pipe_arr[1]) == SYSCALL_FAIL) {
        perror("smash error: close failed");
      }
      if(waitpid(first_cmd_child, nullptr, WUNTRACED) == SYSCALL_FAIL){
        perror("smash error: waitpid failed");
      }
      if(waitpid(second_cmd_child, nullptr, WUNTRACED) == SYSCALL_FAIL){
        perror("smash error: waitpid failed");
      }
    }
  }
}

////////////////////////////////************************** jobs implementation

JobsList::JobsList(): job_map() {}

void JobsList::removeFinishedJobs() {
  if(this->job_map.empty()){
    return;
  }
  for(auto it = this->job_map.begin(); it != this->job_map.end(); ) {
    pid_t result = waitpid(it->second.getJobPid(), nullptr, WNOHANG);  // when child from pipe command run waitpid its fails and erase all jobs
    if(result > 0) {
      it = this->job_map.erase(it);
    }
    else {
      ++it;
    }
  }
}

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped) {
  removeFinishedJobs();
  if(this->job_map.empty()){
    this->job_map.insert(std::make_pair(1, JobsList::JobEntry(cmd, pid, isStopped)));
  }
  else {
    if(cmd->getJobId() !=0) {
      this->job_map.insert(std::make_pair(cmd->getJobId(), JobsList::JobEntry(cmd, pid, isStopped)));
    }
    else{
      this->job_map.insert(std::make_pair(
        (this->job_map.rbegin()->first)+1,
        JobsList::JobEntry(cmd, pid, isStopped)
      ));
    }
  }
}

void JobsList::printJobsList() {
  removeFinishedJobs();
  for(auto& job: this->job_map) {
    std::cout << "[" << job.first << "]" << job.second.getCommand()->getOriginalLine();
    std::cout << " : " << job.second.getJobPid() << " "
    << difftime(time(nullptr), job.second.getTimeCreated()) << " secs";

    if(job.second.getIsStopped()){
      std::cout << " (stopped)";
    }
    std::cout << endl;
  }
}

void JobsList::killAllJobs() {
  removeFinishedJobs();
  std::cout << "smash: sending SIGKILL signal to " << this->job_map.size() << " jobs:" << std::endl;
  for(auto& job: this->job_map) {
    std::cout << job.second.getJobPid() << ": " << job.second.getCommand()->getOriginalLine() << std::endl;
    kill(job.second.getJobPid(), SIGKILL);
  }
}

JobsList::JobEntry& JobsList::getJobById(jid jobId) {
  auto it = this->job_map.find(jobId);
  if(it == this->job_map.end()){
    throw JobIdMissing();
  }
  return it->second;
}

void JobsList::removeJobById(jid jobId) {
  auto it = this->job_map.find(jobId);
  if(it == this->job_map.end()){
    throw(JobIdMissing());
  }
  it->second.getCommand()->setJobId(jobId);
  this->job_map.erase(it);
}

JobsList::JobEntry& JobsList::getLastJob(jid* lastJobId) {
  if(this->job_map.empty()) {
    throw EmptyList();
  }
  *lastJobId = this->job_map.rbegin()->first;
  return this->job_map.rbegin()->second;
}

JobsList::JobEntry& JobsList::getLastStoppedJob(jid *jobId) {
  for(auto i = this->job_map.rbegin(); i != this->job_map.rend(); ++i) {
    if(i->second.getIsStopped()){
      *jobId = i->first;
      return i->second;
    }
  }
  throw NoStoppedJob();
}


////////////////////////////////************************** smash implementation

SmallShell::SmallShell(): prompt("smash"), last_pwd(""), foreground_process(nullptr), fg_pid(NO_FOREGROUND), jobs_list()
{
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}


Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  if(cmd_s.compare("") == 0){
    return nullptr;
  }
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  _removeBackgroundSign(firstWord);
  std::vector<std::string> args;
  _parseCommandLine(cmd_line, args);
  
  if(cmd_s.find_first_of(">") != string::npos){
    return new RedirectionCommand(cmd_line);
  }
  else if(cmd_s.find_first_of("|") != string::npos) {
    return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  else if(firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if(firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, this->last_pwd);
  }
  else if(firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("fare") == 0) {
    return new FareCommand(cmd_line);
  }
  else if(firstWord.compare("setcore") == 0) {
    return new SetcoreCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  this->jobs_list.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  if(cmd){
    cmd->execute();
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}