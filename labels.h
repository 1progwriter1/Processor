struct Label {
    char *name;
    int address;
    int *program_address;
    int prog_adr_size = 1;
    int currant_address;
};

const int START_LABELS_COL = 8;

int SetLabel(Label *labels, char *name, int current_address, int address);