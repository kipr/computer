#ifndef _TARGET_H_
#define _TARGET_H_

#ifdef __cplusplus
extern "C" {
#endif

// QProcess does not correctly emulate a terminal, so output is not flushed by newline.
// This sets flush after every write, which is unfortunately slower.
__attribute__((constructor))
void __set_no_stdout_buffer() {
    setvbuf(stdout, (char *)NULL, _IONBF, 0);
}

#ifdef __cplusplus
}
#endif

#endif
