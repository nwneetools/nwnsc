//
// Created by Kevin Sheehan on 2/10/18.
//

#include <sys/stat.h>
#include "Precomp.h"
#include "OsCompat.h"
#include "easylogging++.h"

std::string
OsCompat::getFileExt(const std::string& s) {

    size_t i = s.rfind('.', s.length());
    if (i != std::string::npos) {
        return(s.substr(i+1, s.length() - i));
    }

    return("");
}

char * OsCompat::filename(const char *str) {
    char *result;
    char *last;
    char *tmpStr = const_cast<char *>(str);
    if ((last = strrchr(tmpStr, '.')) != nullptr ) {
        if ((*last == '.') && (last == tmpStr)) {
            return tmpStr;
        } else {
            result = (char*) malloc(_MAX_FNAME);
            char *fname = strtok(tmpStr,".");
            strncpy(result,fname,_MAX_FNAME);
            return result;
        }
    } else {
        return tmpStr;
    }
}

char * OsCompat::extname(const char *str) {
    char *result;
    char *last;
    char *tmpStr = const_cast<char *>(str);

    if ((last = strrchr(tmpStr, '.')) != nullptr) {
        if ((*last == '.') && (last == tmpStr))
            return "";
        else {
            result = (char*) malloc(_MAX_EXT);
            snprintf(result, _MAX_EXT, "%s", last + 1);
            return result;
        }
    } else {
        return ""; // Empty/nullptr string
    }
}

std::string OsCompat::ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

int OsCompat::dirExists(const char *path)
{
    struct stat info;

    if(stat( path, &info ) != 0) {
        LOG(DEBUG) << "Bad Path";
        return 0;
    }
    else if(info.st_mode & S_IFDIR) {
        LOG(DEBUG) << "Is a dir";
        return 1;
    }
    else {
        LOG(DEBUG) << "Is not a dir";
        return 0;
    }
}
