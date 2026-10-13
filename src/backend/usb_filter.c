#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REAL_USB_BACKEND "/usr/libexec/cups/backend/usb.orig"

int main(int argc, char *argv[]) {
    // If real backend doesn't exist, we cannot do anything
    if (access(REAL_USB_BACKEND, X_OK) != 0) {
        fprintf(stderr, "ERROR: %s is not executable or not found\n", REAL_USB_BACKEND);
        return 1;
    }

    if (argc == 1) {
        // Discovery mode: filter out Canon LBP-810 entries from Apple USB backend
        FILE *fp = popen(REAL_USB_BACKEND, "r");
        if (!fp) {
            return 0;
        }
        char line[4096];
        while (fgets(line, sizeof(line), fp)) {
            // Filter out Canon LBP-810 devices so CUPS/macOS only sees our captusb backend
            if (strstr(line, "Canon/LBP-810") == NULL &&
                strstr(line, "Canon/LBP810") == NULL &&
                strstr(line, "MDL:LBP-810") == NULL) {
                fputs(line, stdout);
            }
        }
        return pclose(fp);
    }

    // Print mode: pass all arguments directly to real Apple USB backend
    argv[0] = (char *)REAL_USB_BACKEND;
    execv(REAL_USB_BACKEND, argv);
    perror("execv " REAL_USB_BACKEND " failed");
    return 1;
}
