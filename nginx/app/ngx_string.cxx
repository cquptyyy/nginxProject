#include <string.h>
void Ltrim(char* string) {
	if (string == nullptr)return;
	if (*string != ' ')return;
	char* ptmp = string;
	while (*ptmp != '\0' && *ptmp == ' ') {
		ptmp++;
	}
	if (*ptmp == '\0') {
		*string = '\0';
		return;
	}
	char* ptmp2 = string;
	while (*ptmp != '\0') {
		*ptmp2++ = *ptmp++;
	}
	*ptmp2 = '\0';
	return;
}
void Rtrim(char* string) {
	size_t len = 0;
	if (string == nullptr)return;
	while (strlen(string)>0 && string[strlen(string) - 1] == ' ') {
		string[strlen(string) - 1] = '\0';
	}
	return;
}