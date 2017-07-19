#ifndef TYPETOSTR_H
#define TYPETOSTR_H
#include <string>
#include <typeinfo>
#include <stdio.h>
#include <sstream>

template<typename F>
std::string typetostr() {
	//cout << (string("echo ")+typeid(F).name()+"c++filt -t").c_str() << endl;
	FILE *pipe = popen((std::string("echo ")+typeid(F).name()+" | c++filt -t").c_str(),"r");
	char buf[200];

	std::ostringstream ss;
	while(!feof(pipe)) {
		if (fgets(buf,199,pipe)!=NULL) {
			int i;
			for(i=0;i<199 && buf[i] && buf[i]!='\n' && buf[i]!='\r';i++)
				;
			if (i<199) buf[i] = 0;
			ss << buf;
		}
	}
	pclose(pipe);
	return ss.str();
}
#endif
