In win8Syscalls.h and win7Syscalls.h are defined the syscalls number for the syscalls needed in the checkpoint restart routine.
The order for the syscalls is the following:
1. WriteFile
2. ReadFile