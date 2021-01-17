# bash-like-shell

This is code for a bash-like shell. This shell has features such as pipelining, it supports multiple single pipline operators(|) in the same command. It also supports double(||) and triple(|||) pipeline operators.

For example:
  $ ls -l || wc, sort
  This is a valid command; it gives two outputs correspondeing to wc and sort.

SIGINT(Ctrl+C) to view the last 10 commands that very given to the shell.

SIGQUIT(Ctrl+\) to exit the shell.

Look at **DesignDoc.pdf** for details about the shell.

**To comiplie: gcc shell.c**
