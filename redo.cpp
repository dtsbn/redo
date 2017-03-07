#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <experimental/optional>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sstream>
#include <dirent.h>

#include <openssl/md5.h>

using namespace std;
using namespace std::experimental;

int write_dep(const string &target, const string &deps_path);

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

optional<string> extension(const string &file) { // return file extension
    auto const pos = file.find_last_of('.');

    if (pos != -1) {
        return file.substr(pos + 1);
    }

    return {};
}

void run_build_script(const string &build_script, const string &target) {
    if (write_dep(target, build_script) != 0) {
        cerr << "Can't create deps metainfo file" << endl;
    }

    const string tmpfile_name(target + ".tmp");
    const string command = "REDO_TARGET=" + target + " sh -e -x " + build_script + " " + target + " " + basename(target) + " " + tmpfile_name;

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

int change_directory(const string &dir) {
    if (chdir(dir.c_str()) == 0) {
        cerr << "Directory changed to " << getcwd() << endl;
        return 0;
    } else {
        cerr << "Couldn't change directory" << endl;
        return 1;
    }
}

int dir_exist(const string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return 1;
    }
    return (info.st_mode & S_IFDIR) == 0;
}

int create_path(const string& path) {
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);

    if (ret == 0) {
        return 0;
    } else {
        cerr << "Can't create path: " << path << endl;
        cerr << "Create path errno: " << errno << endl;
        return 1;
    }
}

string file_md5(const string& path) {
    if (auto fs = ifstream(path, ios::binary | ios::ate)) {
        long filesize = fs.tellg();
        // TODO: handle fileopen exception
        char buf[filesize];
        unsigned char md5_result[MD5_DIGEST_LENGTH];

        fs.seekg(0);
        fs.read(buf, filesize);

        MD5((unsigned char*)buf, filesize, md5_result);

        stringbuf buffer;
        ostream os(&buffer);

        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            os << hex << (int)md5_result[i];
        }

        return buffer.str();
    } else {
        cerr << "Can't open file to calculate md5: " << path << endl;
        return NULL;
    }
}

int write_dep(const string &target, const string &deps_path) {
    if (dir_exist("./.redo") != 0) {
        if (create_path("./.redo") != 0) {
            return 1;
        }
    }

    if (dir_exist("./.redo/" + target) != 0) {
        if (create_path("./.redo/" + target) != 0) {
            return 1;
        }
    }

    string deps_metainfo_path = ".redo/" + target + "/" + file_md5(deps_path);
    cerr << "Creating file " + deps_metainfo_path << endl;
    ofstream myfile(deps_metainfo_path);
    myfile << deps_path << endl;
    myfile.close();

    return 0;
}

int delete_dir(const string& path) {
    std::system(("rm -r ./" + path).c_str());
    return 1;
}

bool upto_date(const string& target) {
    DIR* dir = opendir((".redo/" + target).c_str());

    if (dir == NULL) {
        return false;
    }

    while (auto dp = readdir(dir)) {
        string filename(dp->d_name, dp->d_namlen);

        if (filename == "." || filename == "..") {
            continue;
        }

        if (auto fs = ifstream(".redo/" + target + "/" + filename, ios::binary | ios::ate)) {
            long filesize = fs.tellg();
            char buf[filesize];

            fs.seekg(0);
            fs.read(buf, filesize);

            string dep_filename = string(buf, filesize-1);
            if (filename != file_md5(dep_filename)) {
                return false;
            }

            if (auto build_script = define_build_script(dep_filename)) {
                if (!upto_date(dep_filename)) {
                    closedir(dir);
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    closedir(dir);
    return true;
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

        cerr << "target: " << target << endl;
        cerr << "dir: " << dir << endl << "filename: " << filename << endl;

        if (change_directory(dir) == 1) {
            return 1;
        }

        if (!upto_date(target)) {
            if (delete_dir(".redo/" + target) != 0) {
                cerr << "Can't delete directory: " << ".redo/" + target << endl;
            }

            if (auto build_script = define_build_script(filename)) {
                cerr << "Current build script: " << *build_script << endl;
                cerr << "Calculate md5 of buildscript: " << file_md5(*build_script) << endl;
                run_build_script(*build_script, filename);
            }

            if (ifstream(filename)) {
                if (char* target = getenv("REDO_TARGET")) {
                    cerr << "PARENT_TARGET: " << target << endl;
                    write_dep(target, filename);
                }
            }
        } else {
            cerr << "upto_date == false" << endl;
        }

        if (change_directory(topdir) == 1) {
            return 1;
        }
    }

    return 0;
}
