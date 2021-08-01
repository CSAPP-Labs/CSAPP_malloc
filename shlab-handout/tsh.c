/* 
 * tsh - A tiny shell program with job control
 * 
 * < AN >
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINETSH    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINETSH];         /* for composing sprintf messages */

// Is a foreground process reaped? 1/0
volatile sig_atomic_t fg_reaped;

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINETSH];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);




/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINETSH];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    // Try excluding this when shell is complete.
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINETSH, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard. 

./sdriver.pl -t trace11.txt -s ./tsh -a "-p"

*/
void eval(char *cmdline) 
{

	char *argv[MAXARGS];
	char buf[MAXLINETSH];
	int bg;
	pid_t pid;
	sigset_t mask, prev_mask;

	strcpy(buf, cmdline);
	bg = parseline(buf, argv);
	if (argv[0] == NULL)
		return; /* ignore empty lines; handles case where argc=0 */

	fg_reaped = 0; // set flag to "fg not reaped" at each evaluation

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);

	// if not builtin, run execve in child
	if (!builtin_cmd(argv)) {
		if (!(access((argv[0]), F_OK) == 0)) {
			printf("%s: Command not found\n", argv[0]);
			return;
		}		
  		// block sigchld
		sigprocmask(SIG_BLOCK, &mask, &prev_mask);

		if ((pid = fork()) == 0) {
			// by default, tsh pgid extends to its children. Give separate pgid.
			// so when tsh receives SIGINT, forward it to foreground pgid.
			setpgid(0, 0);

			// unblock sigchld for child process
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);

			// Child process doesn't inherit parent sighandlers.
			// but it does inherit the pending / blocked signal vectors?
			if (execve(argv[0], argv, environ) < 0) {
				printf("%s: Command not found.\n", argv[0]):
				exit(0);
			}
			  
		}
      
	      
		if (!bg) { // at most one FG process

			// in the case a user ctrl+z suspends fg job, the job state
			// can be modified in the list. shouldn't be shown via jobs command
			addjob(jobs, pid, FG, cmdline);

			waitfg(pid); // unblocks sigchld inside

			// unblock sigchld
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);

		} else {
			addjob(jobs, pid, BG, cmdline);

			// sdriver seems to run the echo command first in foreground of tsh.
			printf("[%d] (%d) %s", (getjobpid(jobs, pid))->jid, pid, cmdline);

			// unblock sigchld
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);
		}
			}
		
   
	} 

	// builtin_cmd called any relevant fcn
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINETSH]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line; OK */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately. And return to shell prompt? 
 */
int builtin_cmd(char **argv) 
{

	if (!strcmp(argv[0], "quit")) {
		
		pid_t pid;

		// wait for all children to terminate (NULL), and reap them
		while((pid = waitpid(-1, NULL, 0)) > 0)
			deletejob(jobs, pid);

		// also check if there are no more children via errno!=ECHILD ?

		exit(0);				// exit shell parent process.

	}
   
	// these execute and then return 1 -  eval just returns!
	if (!strcmp(argv[0], "&"))
		return 1;				 

	// uses argv[1] which contains args for fg, bg
	if (!strcmp(argv[0], "fg\0") || !strcmp(argv[0], "bg\0")) {
		do_bgfg(argv);
		return 1;
	}

	// jobs: list all background processes. can't list fg process.
	if (!strcmp(argv[0], "jobs")) {
		listjobs(jobs);
		return 1;
	}

    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
	int raw_num;
	char* str_err = NULL;
	int job = 0;
	char* ID = argv[1];
	pid_t target_pid;
	int jid;

	if (!ID) {
		printf("%s command requires PID or %%jobid argument\n", argv[0]);
		return;
	} 

	// needs to check if the arg after fg/bg is a number, and if it has %
	if ((job = (ID[0] == '%'))) {
		raw_num = strtol(ID+1, &str_err, 10);
		if (*str_err) {
			printf("%s: argument must be a PID or %%jobid\n", argv[0]);
			return;
		}
		jid = raw_num;
		if (!getjobjid(jobs, jid)) {
			printf("%d%%: No such job\n", jid);
			return;
		}
		target_pid = getjobjid(jobs, jid)->pid;
	} else {
		raw_num = strtol(ID, &str_err, 10);
		if (*str_err) {
			printf("%s: argument must be a PID or %%jobid\n", argv[0]);
			return;
		}
		target_pid = raw_num;
		if (!getjobjid(jobs, target_pid)) {
			printf("(%d): No such process\n", target_pid);
			return;
		}
	}


	// fg: restart <job> via kill SIGCONT, run in fg.
	// the fcn call stack should return as do_bgfg->builtin_cmd->eval
	if (!strcmp(argv[0], "fg\0")) {

		if (job) {
			// JID
			getjobjid(jobs, jid)->state = FG;
		} else {
			// PID
			getjobpid(jobs, target_pid)->state = FG;
		}

		// no info upon switching to running FG
		// printf("[%d] (%d) %s", (getjobpid(jobs, target_pid))->jid, target_pid, (getjobpid(jobs, target_pid))->cmdline);
		
		// as with eval, SIGCHLD blocked prior to calling waitfg
		sigset_t mask, prev_mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);
		sigprocmask(SIG_BLOCK, &mask, &prev_mask);

		kill(-target_pid, SIGCONT);
		waitfg(target_pid);

		sigprocmask(SIG_SETMASK, &prev_mask, NULL);

	} else {
	// bg: restart <job> via kill SIGCONT, run in bg.
	// if (!strcmp(argv[0], "bg\0")) {
		if (job) {
			// JID
			getjobjid(jobs, jid)->state = BG;
		} else {
			// PID
			getjobpid(jobs, target_pid)->state = BG;
		}

		// cleanup taken care of by SIGCHLD handler
		printf("[%d] (%d) %s", (getjobpid(jobs, target_pid))->jid, target_pid, (getjobpid(jobs, target_pid))->cmdline);
		kill(-target_pid, SIGCONT);

	}


    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) // arg pid used when waitfg called by fg<job> ?
{

	sigset_t empty_mask;
	sigemptyset(&empty_mask);

	// if one of the processes the SIGCHLD handler reaped is the 
	// leading foreground child process, the handler updates the
	// flag and this loop exits 
	while (!fg_reaped && (fgpid(jobs))) 
		sigsuspend(&empty_mask); // periodically unblock SIGCHLD

	fg_reaped = 0; // reset flag back to fg not reaped
   
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) {
	
	pid_t pid_reaped;
	pid_t fg_pid = fgpid(jobs);
	int status;

	// reap all available terminated children, report on suspended children
	while((pid_reaped = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

		if (WIFSTOPPED(status)){
			
			// this should only run if stopped by signal not from shell; 
			// or if state not already updated by sigtstp handler
			if (getjobpid(jobs, pid_reaped)->state != ST) {
				getjobpid(jobs, pid_reaped)->state = ST;
				printf("Job [%d] (%d) stopped by signal %d\n", (getjobpid(jobs, pid_reaped))->jid, pid_reaped, WSTOPSIG(status));
			}
			// return;
		} else {
			if (pid_reaped == fg_pid) { // the fg process child is reaped
				fg_reaped = 1; 
			} 

			// process terminated because of a signal not caught, by the child?
			// so this status says that the CHILD did not catch it?
			if (WIFSIGNALED(status)) { 
				printf("Job [%d] (%d) terminated by signal %d\n", (getjobpid(jobs, pid_reaped))->jid, pid_reaped, WTERMSIG(status));
			}

			deletejob(jobs, pid_reaped);
		}

	}

    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
	pid_t fg_pid = fgpid(jobs);

	// message. Note: deletion of job is done by SIGCHLD handler.
	// printf("Job [%d] (%d) terminated by signal %d\n", (getjobpid(jobs, fg_pid))->jid, fg_pid, sig);

	// forward SIGINT to fg job
	kill(-fg_pid, SIGINT);
	
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
	pid_t fg_pid = fgpid(jobs);
	// forward SIGTSTP to fg job's entire process group
	kill(-fg_pid, SIGTSTP);
	// update jobs list to reflect this. should be read by waitfg()
	getjobpid(jobs, fg_pid)->state = ST;

	// message. how does SIGCHLD handler behave here?
	printf("Job [%d] (%d) stopped by signal %d\n", (getjobpid(jobs, fg_pid))->jid, fg_pid, sig);

    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list of size MAXJOBS */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



