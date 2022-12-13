#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
	cout << "smash: got ctrl-Z" << endl;
  pid_t fg_process = SmallShell::getInstance().getFgPid();
  if(fg_process != NO_FOREGROUND){
    SmallShell::getInstance().getJobsList().addJob(
      SmallShell::getInstance().getForegroundProcess(),
      fg_process,
      true);
    if(kill(fg_process, SIGSTOP) == SYSCALL_FAIL){
        perror("smash error: kill failed");
      }
    cout << "smash: process " << fg_process << " was stopped" << endl;
  }
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  pid_t fg_process = SmallShell::getInstance().getFgPid();
  if(fg_process != NO_FOREGROUND) {
    if(kill(fg_process, SIGKILL) == SYSCALL_FAIL){
      perror("smash error: kill failed");
    }
    cout << "smash: process " << fg_process << " was killed" << endl;
    SmallShell::getInstance().setFgPid(NO_FOREGROUND);
    SmallShell::getInstance().setForegroundProcess(nullptr);
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

