#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <experimental/optional>
#include <unistd.h>

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

        if (chdir(dir.c_str()) == 0) {
            cerr << "Directory changed to " << getcwd() << endl;
        } else {
            cerr << "Couldn't change directory" << endl;
            return 1;
        }

        if (auto build_script = define_build_script(filename)) {
            cerr << "Current build script: " << *build_script << endl;
            run_build_script(*build_script, filename);
        } else {
            cerr << "Can't find build script" << endl;
            return 1;
        }

        chdir(topdir.c_str());
    }

    return 0;
}
