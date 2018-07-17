#simple build script if cmake is not installed.
OBJS="listener.o connmanager.o connection.o listener.h.gch connmanager.h.gch connection.h.gch"
rm ${OBJS}

SRCS="main.cpp listener.cpp connmanager.cpp connection.cpp"
g++ -Wall -o server -std=c++14 ${SRCS} -I/usr/local/include -lboost_system -lboost_program_options
