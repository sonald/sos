#ifndef _STD_ARG_H
#define _STD_ARG_H 

#define __va_rounded_size(TYPE)  \
      (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

typedef char* va_list;
#define va_start(ap, last) (ap = (((char*)&last) + __va_rounded_size(last)))
#define va_arg(ap, type) ({      \
        type r = *(type*)ap;     \
        ap = ap + __va_rounded_size(type); \
        r;                       \
        }) 

#define va_end(ap) (ap = NULL)


#endif
