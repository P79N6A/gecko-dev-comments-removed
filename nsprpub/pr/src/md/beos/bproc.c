




































#include "primpl.h"
#include <stdio.h>
#include <signal.h>

#define _PR_SIGNALED_EXITSTATUS 256

PRProcess*
_MD_create_process (const char *path, char *const *argv,
		    char *const *envp, const PRProcessAttr *attr)
{
	PRProcess *process;
	int nEnv, idx;
	char *const *childEnvp;
	char **newEnvp = NULL;
	int flags;
	PRBool found = PR_FALSE;

	process = PR_NEW(PRProcess);
	if (!process) {
		PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
		return NULL;
	}

	childEnvp = envp;
	if (attr && attr->fdInheritBuffer) {
		if (NULL == childEnvp) {
			childEnvp = environ;
		}
		for (nEnv = 0; childEnvp[nEnv]; nEnv++) {
		}
		newEnvp = (char **) PR_MALLOC((nEnv + 2) * sizeof(char *));
		if (NULL == newEnvp) {
			PR_DELETE(process);
			PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
			return NULL;
		}
		for (idx = 0; idx < nEnv; idx++) {
			newEnvp[idx] = childEnvp[idx];
			if (!found && !strncmp(newEnvp[idx], "NSPR_INHERIT_FDS=", 17)) {
				newEnvp[idx] = attr->fdInheritBuffer;
				found = PR_TRUE;
			}
		}
		if (!found) {
			newEnvp[idx++] = attr->fdInheritBuffer;
		}
		newEnvp[idx] = NULL;
		childEnvp = newEnvp;
	}

	process->md.pid = fork();

	if ((pid_t) -1 == process->md.pid) {
		PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, errno);
		PR_DELETE(process);
		if (newEnvp) {
			PR_DELETE(newEnvp);
		}
		return NULL;
	} else if (0 == process->md.pid) {  
		






		if (attr) {
			
			int in_osfd = -1, out_osfd = -1, err_osfd = -1;

			if (attr->stdinFd
			    && attr->stdinFd->secret->md.osfd != 0) {
				in_osfd = attr->stdinFd->secret->md.osfd;
				if (dup2(in_osfd, 0) != 0) {
					_exit(1);  
				}
				flags = fcntl(0, F_GETFL, 0);
				if (flags & O_NONBLOCK) {
					fcntl(0, F_SETFL, flags & ~O_NONBLOCK);
				}
			}
			if (attr->stdoutFd
			    && attr->stdoutFd->secret->md.osfd != 1) {
				out_osfd = attr->stdoutFd->secret->md.osfd;
				if (dup2(out_osfd, 1) != 1) {
					_exit(1);  
				}
				flags = fcntl(1, F_GETFL, 0);
				if (flags & O_NONBLOCK) {
					fcntl(1, F_SETFL, flags & ~O_NONBLOCK);
				}
			}
			if (attr->stderrFd
			    && attr->stderrFd->secret->md.osfd != 2) {
				err_osfd = attr->stderrFd->secret->md.osfd;
				if (dup2(err_osfd, 2) != 2) {
					_exit(1);  
				}
				flags = fcntl(2, F_GETFL, 0);
				if (flags & O_NONBLOCK) {
					fcntl(2, F_SETFL, flags & ~O_NONBLOCK);
				}
			}
			if (in_osfd != -1) {
				close(in_osfd);
			}
			if (out_osfd != -1 && out_osfd != in_osfd) {
				close(out_osfd);
			}
			if (err_osfd != -1 && err_osfd != in_osfd
			    && err_osfd != out_osfd) {
				close(err_osfd);
			}
			if (attr->currentDirectory) {
				if (chdir(attr->currentDirectory) < 0) {
					_exit(1);  
				}
			}
		}

		if (childEnvp) {
			(void)execve(path, argv, childEnvp);
		} else {
			
			(void)execv(path, argv);
		}
		
		_exit(1);
	}

	if (newEnvp) {
		PR_DELETE(newEnvp);
	}

	return process;
}

PRStatus
_MD_detach_process (PRProcess *process)
{
	



	PR_DELETE(process);
	return PR_SUCCESS;
}

PRStatus
_MD_wait_process (PRProcess *process, PRInt32 *exitCode)
{
	PRStatus retVal = PR_SUCCESS;
	int ret, status;
	
	
	do {
		ret = waitpid(process->md.pid, &status, 0);
	} while (ret == -1 && errno == EINTR);

	


 
	PR_ASSERT(0 != ret);

	if (ret < 0) {
                PR_SetError(PR_UNKNOWN_ERROR, _MD_ERRNO());
		return PR_FAILURE;
	}

	
	if (WIFEXITED(status)) {
		*exitCode = WEXITSTATUS(status);
	} else {
		PR_ASSERT(WIFSIGNALED(status));
		*exitCode = _PR_SIGNALED_EXITSTATUS;
	}		

	PR_DELETE(process);
	return PR_SUCCESS;
}

PRStatus
_MD_kill_process (PRProcess *process)
{
	PRErrorCode prerror;
	PRInt32 oserror;
	
	if (kill(process->md.pid, SIGKILL) == 0) {
		return PR_SUCCESS;
	}
	oserror = errno;
	switch (oserror) {
        case EPERM:
		prerror = PR_NO_ACCESS_RIGHTS_ERROR;
		break;
        case ESRCH:
		prerror = PR_INVALID_ARGUMENT_ERROR;
		break;
        default:
		prerror = PR_UNKNOWN_ERROR;
		break;
	}
	PR_SetError(prerror, oserror);
	return PR_FAILURE;
}
