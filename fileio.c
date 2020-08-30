#include"common.h"

char* mystrstr(char* str, const char* const find){
    int i = 0;
    int j = 0;
    int found = 0;

    do{
        if(find[j] == '\0') break;

        if(found == 0 && str[i] == find[j]){
            found = i;
            j++;
        }
        else if(str[i] == find[j]){
            j++;
        }
        else if(found != 0 && str[i] != find[j]){
            found = 0;
            j = 0;
        }

    }while(str[i++]);

    char* temp = malloc(found * sizeof(char));

    for(int i = 0; i < found; i++) temp[i] = str[i];
    return temp;
}

int main(int argc, char* argv[]){
    /*int fd = open("/tmp/file", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    assert(fd > -1);
    int rc = write(fd, "hello world\n", 13);
    assert(rc == 13);
    close(fd);
*/
    const char* str = "helloworldtest";
    char* temp = mystrstr(str, "world");

    printf("%s\n", temp);

    return 0;

}

