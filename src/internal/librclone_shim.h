#ifndef LIBRCLONE_SHIM_H
#define LIBRCLONE_SHIM_H

typedef struct {
    char* Output;
    int   Status;
} RcloneRPCResult;

int librclone_ensure_loaded(void);

void            RcloneInitialize(void);
void            RcloneFinalize(void);
RcloneRPCResult RcloneRPC(char* method, char* input);
void            RcloneFreeString(char* str);

#endif
