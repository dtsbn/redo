#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <experimental/optional>

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

string basename(const string &name) {
    auto const pos = name.find_last_of('.');

    return name.substr(0, pos);
}

optional<string> extension(const string &file) {
    auto const pos = file.find_last_of('.');

    if (pos != file.length()) {
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

    for (int i = 1; i < argc; i++) {
        string target(argv[i]);

        if (auto build_script = define_build_script(target)) {
            cout << "Current build script: " << *build_script << endl;
            run_build_script(*build_script, target);
        } else {
            cerr << "Can't find build script" << endl;
        }
    }

    return 0;
}
