#include <string>
#include <vector>



int aftername(std::string name, int border = 4){
    return (name.length() + border + 1);
}

std::vector<std::string> split(const std::string& str, char separator) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = str.find(separator, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}
