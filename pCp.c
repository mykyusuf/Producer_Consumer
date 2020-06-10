#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#define BUFSIZE 100

pthread_mutex_t readmutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writemutex=PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t emptycond=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fullcond=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t stackcond=PTHREAD_COND_INITIALIZER;
pthread_cond_t stackcond2=PTHREAD_COND_INITIALIZER;

struct Stack* buffer;
int con=1;

void writer(char* t,char* source,char* dest){
    
    FILE* F2;
    char str[256];
    char cwd[256];
    getcwd(cwd,sizeof(cwd));
    chdir(source);
    
    F2=fopen(t,"r");
    fread(str,sizeof(str),256,F2);
    fclose(F2);
    
    chdir(cwd);
    chdir(dest);
    F2=fopen(t,"w");
    fprintf(F2,"%s\n",str);
    fclose(F2);
    
    chdir(cwd);
    
}


struct Stack
{
    int top;
    unsigned capacity;
    char** array;
};

struct Stack* createStack(unsigned capacity)
{
    struct Stack* stack = (struct Stack*) malloc(sizeof(struct Stack));
    stack->capacity = capacity;
    stack->top = -1;
    stack->array = (char**) malloc(stack->capacity * sizeof(char));
    return stack;
}

int isFull(struct Stack* stack)
{ return stack->top == stack->capacity - 1; }

int isEmpty(struct Stack* stack)
{ return stack->top == -1; }

void push(struct Stack* stack, char* item)
{
    if (isFull(stack))
        return;
    stack->array[++stack->top] = item;
}

char* pop(struct Stack* stack)
{
    if (isEmpty(stack))
        return NULL;
    return stack->array[stack->top--];
}


int isDir(char *path){
    struct stat s_buffer;
    if(stat(path, &s_buffer) == -1)
        return 0;
    else
        return S_ISDIR(s_buffer.st_mode);
}

void* Producer(void* input){
    char** argv=(char**) input;
    
    char srcPath[50];
    strcat(srcPath, argv[3]);
    char dstPath[50];
    strcat(dstPath, argv[4]);
    
    struct dirent *createdPath;
    DIR *dPtr;
    dPtr = opendir(srcPath);
    FILE* F;
    
    if(isDir(srcPath))
    {
        char src[100];
        char dst[100];
        strcpy(src,srcPath);
        strcpy(dst,dstPath);
        while((createdPath = readdir(dPtr))!=NULL)
        {
            if(strcmp(createdPath->d_name, ".")!=0 && strcmp(createdPath->d_name, "..")!=0)
            {

                strcat(dstPath,"/");
                strcat(dstPath, createdPath->d_name);
                
                pthread_mutex_lock(&readmutex);
                
                if(isFull(buffer))
                    pthread_cond_wait(&stackcond, &fullcond);
                
                F=fopen(dstPath,"w");
                push(buffer,createdPath->d_name);
                
                printf("producer içi...%s\n",createdPath->d_name);
                fclose(F);
                
                pthread_mutex_unlock(&readmutex);
                pthread_cond_signal(&stackcond2);
                
                strcpy(srcPath,src);
                strcpy(dstPath,dst);
                
            }
        }
        con=0;
    }
    
    exit(0);
    
}

void* Consumer(void* bff){
    char** tmp=(char**)bff;

    while(con){
        
        pthread_mutex_lock(&writemutex);

        if(isEmpty(buffer))
            pthread_cond_wait(&stackcond2, &emptycond);
        else{
            char* t=pop(buffer);
            
            writer(t,tmp[3],tmp[4]);
            
            pthread_cond_signal(&stackcond);

        }
        pthread_mutex_unlock(&writemutex);
        
        
    }
    exit(0);
    
}


int main(int argc, char* argv[])
{
    buffer = createStack(atoi(argv[2]));
    char** x1=argv;
    time_t seconds;
    
    if(argc==5){
    
    pthread_t* threads=(pthread_t*)malloc(atoi(argv[1]));
    pthread_t threadPro;
    pthread_create(&threadPro, NULL, Producer, &argv[0]);
    
    for(int j=0;j<atoi(argv[2])+1;j++){
    
        pthread_create(&threads[j], NULL, Consumer, &x1[0]);
    
    }
    for(int j=0;j<atoi(argv[2])+1;j++){
        
        pthread_join(threads[j],NULL);

    }

    pthread_join(threadPro,NULL);
    
    
    free(threads);
    pthread_mutex_destroy(&readmutex);
    pthread_mutex_destroy(&writemutex);
    pthread_mutex_destroy(&fullcond);
    pthread_mutex_destroy(&emptycond);
        
    seconds = time (NULL);
        
    printf ("gecen sure %ld \n", seconds/3600);
    }
    else{
        printf("Wrong Usage! ./exe threadCount bufferSize sourcePath destPath");
    }
    
    return 0;
}
