
#ifndef SIECI2_ERR_H

#include <string>

/* Wypisuje informację o błędnym zakończeniu funkcji systemowej
i kończy działanie programu. */
extern void syserr(const char *fmt, ...);

/* Wypisuje informację o błędzie i kończy działanie programu. */
extern void fatal(const char *fmt, ...);

//void fatal(std::string info);
//void fatal1(std::string info){
//    fprintf(stderr, "Error: %s\n", info);
//    exit(EXIT_FAILURE);
//}

#endif //SIECI2_ERR_H