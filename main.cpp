#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <iostream>



//Create a function that receives a file and reads it skipping the first line and returns a string containing the file
std::string readFile(std::string fileName){
    std::ifstream file(fileName);
    std::string line;
    std::string fileContent;
    if(file.is_open()){
        getline(file, line);
        while(getline(file, line)){
            fileContent += line + "\n";
        }
        file.close();
    }
    return fileContent;
}


int main(){

    std::string fileContent = readFile("genoma.fna");

    return 0;
}