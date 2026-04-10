## Context-switching approach
I implemented user level threads in user space. Each thread has its own stack and saved register context. The 'uswtch' function saves the current thread registers and stack pointer, then restores the next thread's saved context. Thread switching happens only when a thread calls thread_yield().

## Limitations
- Maximum number of threads: 16
- Stack size per thread: 4096 bytes
- Scheduling is cooperative only, so threads must call thread_yield()
- The mutex is a simple cooperative mutex and depends on cooperative scheduling