#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <experimental/optional>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;
using namespace std::experimental;

optional<string> extension(const string &file);
string basename(const string &name);

optional<string> define_build_script(const string &target) {
    auto const script = target + ".do";

    if (ifstream(script)) {
        return script;
    } else if (auto ext = extension(target)) {
        auto const default_script = "default." + *ext + ".do";

        if (ifstream(default_script)) {
            return default_script;
        }
    }

    return {};
}

string getcwd() {
    char buf[FILENAME_MAX];
    char* succ = getcwd(buf, FILENAME_MAX);

    if (succ) {
        return string(succ);
    }

    return "";
}

string basename(const string &name) {
    auto const pos = name.find_last_of('.');

    return name.substr(0, pos);
}

pair<string, string> split_filename(const string &path) {
    auto const pos = path.find_last_of('/');

    if (pos == -1) {
        return make_pair("./", path);
    }

    return make_pair(path.substr(0, pos), path.substr(pos + 1));
}

optional<string> extension(const string &file) {
    auto const pos = file.find_last_of('.');

    if (pos != -1) {
        return file.substr(pos + 1);
    }

    return {};
}

void run_build_script(const string &build_script, const string &target) {
    const string tmpfile_name(target + ".tmp");
    const string command = "sh -e " + build_script + " " + target + " " + basename(target) + " " + tmpfile_name;

    cerr << "Run build script command: " << command << endl;

    cerr << "redo " + target << endl;
    if (std::system(command.c_str()) == 0) {
        cerr << "Running build script: success" << endl;

        if (ifstream(tmpfile_name, ios::binary | ios::ate).tellg() != 0) {
            std::rename(tmpfile_name.c_str(), target.c_str());
        } else {
            std::remove(tmpfile_name.c_str());
        }
    } else {
        cerr << "Running build script: fail" << endl;
    }
}

int create_deps_file(const string &target, const string &deps_path) {
    // to be implemented
}

int change_directory(const string &dir) {
    if (chdir(dir.c_str()) == 0) {
        cerr << "Directory changed to " << getcwd() << endl;
        return 0;
    } else {
        cerr << "Couldn't change directory" << endl;
        return 1;
    }
}

int dir_exist(const string& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return 1;
    }
    return (info.st_mode & S_IFDIR) == 0;
}

int create_path(const string& path)
{
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);

    if (ret == 0) {
        return 0;
    } else {
        cerr << "Create path errno: " << errno << endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Wrong arguments" << endl;
        return 1;
    }

    auto topdir = getcwd();
    cerr << "Top directory: " << topdir << endl;

    for (int i = 1; i < argc; i++) {
        string target(argv[i]);
        string dir, filename;
        tie(dir, filename) = split_filename(target);

        cerr << "dir: " << dir << endl << "filename: " << filename << endl;

        if (change_directory(dir) == 1) {
            return 1;
        }

        string path_to_deps = "./.redo/" + filename ;

        if (dir_exist("./.redo") != 0) {
            create_path("./.redo");
        }

        if (dir_exist(path_to_deps) != 0) {
            if (create_path(path_to_deps) == 0) {
                cerr << "Path successfyly created: " << path_to_deps << endl;
            } else {
                cerr << "Can't create path: " << path_to_deps << endl;
                return 1;
            }
        }

        if (auto build_script = define_build_script(filename)) {
            cerr << "Current build script: " << *build_script << endl;
            run_build_script(*build_script, filename);
        } else {
            cerr << "Can't find build script" << endl;
            return 1;
        }

        if (change_directory(topdir) == 1) {
            return 1;
        }
    }

    return 0;
}
