#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

// Function to split a string into a vector of strings
vector<string> split(const string& str, char delim) {
    vector<string> tokens;
    string token;
    for (char c : str) {
        if (c == delim) {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    tokens.push_back(token);
    return tokens;
}

// Function to execute a command
void executeCommand(const string& command) {
    vector<string> args = split(command, ' ');
    char* argArray[args.size() + 1];
    for (int i = 0; i < args.size(); i++) {
        argArray[i] = strdup(args[i].c_str());
    }
    argArray[args.size()] = nullptr;

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(argArray[0], argArray);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, nullptr, 0);
    } else {
        perror("fork");
        exit(1);
    }

    // Free the memory allocated for argArray
    for (int i = 0; i < args.size(); i++) {
        free(argArray[i]);
    }
}

// Function to autocomplete using readline
char* generator(const char* text, int state) {
    // Generate a list of possible completions
    static DIR* dir;
    static int count;
    static struct dirent* dent;

    if (!state) {
        dir = opendir(".");
        count = 0;
    }

    while ((dent = readdir(dir)) != NULL) {
        count++;
    }

    closedir(dir);

    return strdup(dent->d_name);
}

char** autocomplete(const char* text, int start, int end) {
    if (text == NULL) {
        return NULL; // or some other error handling
    }

    // Create a list of possible completions
    char** matches = rl_completion_matches(text, generator);

    // Return the list of matches
    return matches;
}

// Function to autocomplete using fzf
char** autocompleteFzf(const char* text, int start, int end) {
    string command = "fzf -q '" + string(text) + "'";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return nullptr;
    }
    char buffer[128];
    vector<string> matches;
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            string result = buffer;
            result.pop_back(); // Remove newline character
            matches.push_back(result);
        }
    }
    pclose(pipe);
    char** result = new char*[matches.size() + 1];
    for (int i = 0; i < matches.size(); i++) {
        result[i] = strdup(matches[i].c_str());
    }
    result[matches.size()] = nullptr;
    return result;
}

int main() {
    extern char **environ;
    setenv("PATH", "/bin:/usr/bin:/usr/local/bin", 1);
    rl_attempted_completion_function = autocomplete;
    while (true) {
        char* line = readline("dash> ");
        if (line && *line) {
            add_history(line);
            if (strcmp(line, "exit") == 0) {
                break;
            }
            if (strncmp(line, "cd ", 3) == 0) {
                string path = line + 3;
                if (chdir(path.c_str()) != 0) {
                    perror("chdir");
                }
            } else if (strcmp(line, "shift+tab") == 0) {
                rl_attempted_completion_function = autocompleteFzf;
            } else {
                executeCommand(line);
            }
        }
        free(line);
    }
    return 0;
}