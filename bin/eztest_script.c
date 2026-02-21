setlinebuf(stdout);

printf("I eztest_script: starting\n");
int rc;
rc = sdlx_init(SUBSYS_VIDEO);
if (rc != 0) {
    printf("E %s: sdlx_init failed\n", progname);
    return 1;
}

char *argv[2];
argv[0] = "PROGNAME";
argv[1] = "DATA_DIR";
main(2, argv);

printf("I eztest_script: terminating\n");
