typedef struct {
	char *ip;
	int port;
	char *filename;
}ota_parameters_t;

void ota_main(char *ip, int port, char *filename);