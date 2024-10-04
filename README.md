# **Prompt Shell Application**

Basic Linux shell that executes commands using standard I/O, while also being able to run commands in background. This shell implements fundamental concepts and techniques of Operating Systems, including system calls, processes, forking, piping, file descriptors etc. An input will be read by the shell and mimic the behaviour of a real-life Linux shell, accepting commands that are background commands, redirection commands (commands that read from or write to a file) and piped commands.
 
## Command struct class
A Command struct class was used for the commands that will be typed by stdin into the shell, which has the following attributes:
* `name`: The command name itself, such as `ls`, `cat`, `echo`...
* `argv`: The rest of the arguments in the command that may include flags put into an array
* `isBackground`: boolean variable that determines whether the command is a background command (&)
* `backgroundJobIndex`: If it is a background command, this variable determines the current index of the background commands running at the same time
* `isInputRedirect`: boolean variable that determines if the command reads input from a file (<)
* `inputFile`: If it does read input from a file, this variable will be the inputted file
* `isOutputRedirect`: boolean variable that determines if the command writes output to a file (>)
* `outputFile`: If it does write output to a file, this variable will be the outputted file

## commands.c functions
The `commands.c` file contains all the command related functions as listed below: 
1. `printCommand`: Prints name and arguments of a Command object, used for debugging. 
2. `trimWhiteSpace`: Loops through a string to remove any leading or trailing whitespaces.
3. `parseCommand`: Converts a command inputted as a string into a Command object
- An empty Command object was initialised with all its attributes
- The input is read, and the code will handle any failures in memory allocation for the string or the array of argv
- `strchr` was used to locate the first instance of '>' or '<' meta-characters to determine if the command will read from or write to a file
- If it is a redirection command, the command's respective redirect boolean attribute and file attribute will be set accordingly
- While going through the command names and argv's, the `trimWhitespace` function is used to remove whitespaces before being set to the command's attrubutes.
- The final character of the input string is checked if it contains the '&' meta-character so `isBackground` attribute can be set to true.
4. `splitByPipe`: Takes in an input string of multiple piped commands and adjusts an array of commands to be manipulated via pointers, returning a count of piped commands as output
- Loop through every '|' separated command
- Trim the whitespaces and increment the total piped commands counter
- Add to the array of piped commands
- Return the total piped commands count
5. `backgroundProcessHandler`: Signal handler for a background process
- Return if no `SIGCHLD` signal was received
- Loop through all the child processes using `waitpid` to reap child processes. If a child process has terminated, `waitpid` returns its PID, and the loop continues.
- If the child process is terminated normally, update the background job counter variable by decrementing and print the job completion information
6. `executeRedirectCommand`
- If the command being executed writes to a file (>):
    - Extract the output file from the command object's attribute
    - Create a file descriptor using the `open` command with `O_WRONLY`, `O_CREAT` and `O_TRUNC` with appropriate file permissions
    - Use `dup2` to create a copy of the fd and redirect from stdout, closing the original fd
- If the command reads from a file (<):
    - Extract the input file from the command object's attribute
    - Create fd using the `open` command with `O_RDONLY`
    - Use `dup2` to fd copy and redirect stdin, closing the original fd
- If the param `isExec` is true, `execvp` and its associated error-checking lines will be run
7. `executeSingleCommand`:
  If the command is normal (e.g. `echo hello`) or redirection (e.g. `ls > foo`), child processes are used to interact with fd's. If the command is background (e.g. sleep 10 &), parent processes are used without waiting for child processes.
- Initialise pid and wait status variables
- Use forking to create child and parent processes
- For child process:
    - If the command is a redirection command, use `executeRedirectCommand` function
    - Else, run `execvp` and check for any errors
- For parent process:
    - If the command is a background command:
        - Update the `Command.backgroundJobIndx`
        - Update `lastBackgroundCommand` to be the current command
        - Release the SIGCHLD signal when child process terminates, to be received by the `backgroundProcessHandler` to handle the signal and clean up finished background processes
    - Else:
        - Set SIGCHLD signal handler to its default setting to terminate the parent process if a child exits
        - Use `waitpid` to wait for the child to terminate
        - Release `SIGCHLD` to signal handler
8. `executePipedCommands`
  - Initialise pid, current command and previous fd variables
  - Loop through the command count to access and parse each command
  - Use `pipefd[2]` for read-end and write-end
  - Fork to create child process and parent process.
  - In the child process:
      - If the piped command is redirection, run `execRedirectCommand`
      - If the previous fd is not -1, use dup2 to redirect stdin to read from `prev_fd` and close the original prev_fd
      - If the command is "internal" (not the last command), use piping to close the read-end of the pipe (`pipefd[0]`), redirect stdout to write-end of the pipe (`pipefd[1]`) and close that afterwards
      - Run `execvp` to execute the command
  - In the parent process:
      - Initialise wait status and set `SIGCHLD` to default settings
      - Wait for child to terminate
      - If `prev_fd != -1`, close prev_fd
      - If not the last command, close the write-end and set prev_fd to be read_end

## myshell.c functions
1. `runShell`
  - Use an input buffer to read in the input and initialise an array of commands
  - In an infinite `while(true)` loop:
      - Ensure the `prompt_shell$ ` string is printed depending on whether there is a `-n` flag
      - Read in the input, and use `splitByPipe` to manipulate the array of commands
      - If there are multiple piped commands, trim each command and run `executePipedCommands`
      - Otherwise, parse the first command and run `executeSingleCommand`

In the main function, we simply check whether there is a `-n` flag and use `runShell`.

