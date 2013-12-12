/* many macros for debuging */
#define MESSAGE(info) fprintf(stderr, info "\n")
#define MESSAGE_S(info, str) fprintf(stderr, info "%s\n", str)
#define MESSAGE_D(info, i) fprintf(stderr, info "%d\n", i)
#define TOSTRING(n) #n
#define LOG_ERR(info, log) ngx_log_error(NGX_LOG_ERR, log, 0, info __FILE__ ":L" TOSTRING(__LINE__))

#define DEBUG_FLAG 1

#define IFDEBUG(block) do { if (DEBUG_FLAG) { block; } } while(0)

#define PRINT_INFO(info) IFDEBUG( fprintf(stderr, info "\n") )
#define PRINT_INT(info, i) IFDEBUG( fprintf(stderr, info "%d\n", (int)i) )
#define PRINT_FLOAT(info, f) IFDEBUG( fprintf(stderr, info "%f\n", f) )
#define PRINT_STR(info, str) IFDEBUG( fprintf(stderr, info " #%s#\n", str) )
#define PRINT_NGXSTR(info, nstr) IFDEBUG( fprintf(stderr, info " #%.*s#\n", (int)nstr.len, nstr.data) )
#define PRINT_NGXSTR_PTR(info, nstr_ptr) IFDEBUG( fprintf(stderr, info " #%.*s#\n", (int)nstr_ptr->len, nstr_ptr->data) )
