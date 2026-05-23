typedef __builtin_va_list __gnuc_va_list;
typedef __builtin_va_list va_list;
typedef long int ptrdiff_t;
typedef long unsigned int size_t;
typedef int wchar_t;
typedef struct
{
    long long __clang_max_align_nonce1 __attribute__((__aligned__(__alignof__(long long))));
    long double __clang_max_align_nonce2 __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;
typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct
{
    int __val[2];
} __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;
typedef int __daddr_t;
typedef int __key_t;
typedef int __clockid_t;
typedef void * __timer_t;
typedef long int __blksize_t;
typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;
typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;
typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;
typedef long int __fsword_t;
typedef long int __ssize_t;
typedef long int __syscall_slong_t;
typedef unsigned long int __syscall_ulong_t;
typedef __off64_t __loff_t;
typedef char * __caddr_t;
typedef long int __intptr_t;
typedef unsigned int __socklen_t;
typedef int __sig_atomic_t;
typedef struct
{
    int __count;
    union
    {
        unsigned int __wch;
        char __wchb[4];
    } __value;
} __mbstate_t;
typedef struct _G_fpos_t
{
    __off_t __pos;
    __mbstate_t __state;
} __fpos_t;
typedef struct _G_fpos64_t
{
    __off64_t __pos;
    __mbstate_t __state;
} __fpos64_t;
struct _IO_FILE;
typedef struct _IO_FILE __FILE;
struct _IO_FILE;
typedef struct _IO_FILE FILE;
struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;
typedef void _IO_lock_t;
struct _IO_FILE
{
    int _flags;
    char * _IO_read_ptr;
    char * _IO_read_end;
    char * _IO_read_base;
    char * _IO_write_base;
    char * _IO_write_ptr;
    char * _IO_write_end;
    char * _IO_buf_base;
    char * _IO_buf_end;
    char * _IO_save_base;
    char * _IO_backup_base;
    char * _IO_save_end;
    struct _IO_marker * _markers;
    struct _IO_FILE * _chain;
    int _fileno;
    int _flags2;
    __off_t _old_offset;
    unsigned short _cur_column;
    signed char _vtable_offset;
    char _shortbuf[1];
    _IO_lock_t * _lock;
    __off64_t _offset;
    struct _IO_codecvt * _codecvt;
    struct _IO_wide_data * _wide_data;
    struct _IO_FILE * _freeres_list;
    void * _freeres_buf;
    size_t __pad5;
    int _mode;
    char _unused2[15 * sizeof(int) - 4 * sizeof(void *) - sizeof(size_t)];
};
typedef __ssize_t cookie_read_function_t(void * __cookie, char * __buf, size_t __nbytes);
typedef __ssize_t cookie_write_function_t(void * __cookie, char const * __buf, size_t __nbytes);
typedef int cookie_seek_function_t(void * __cookie, __off64_t * __pos, int __w);
typedef int cookie_close_function_t(void * __cookie);
typedef struct _IO_cookie_io_functions_t
{
    cookie_read_function_t * read;
    cookie_write_function_t * write;
    cookie_seek_function_t * seek;
    cookie_close_function_t * close;
} cookie_io_functions_t;
typedef __gnuc_va_list va_list;
typedef __off_t off_t;
typedef __ssize_t ssize_t;
typedef __fpos_t fpos_t;
extern FILE * stdin;
extern FILE * stdout;
extern FILE * stderr;
extern int remove(char const * __filename) __attribute__((__nothrow__));
extern int rename(char const * __old, char const * __new) __attribute__((__nothrow__));
extern int renameat(int __oldfd, char const * __old, int __newfd, char const * __new) __attribute__((__nothrow__));
extern int fclose(FILE * __stream) __attribute__((__nonnull__(1)));
extern FILE * tmpfile(void) __attribute__((__malloc__));
extern char * tmpnam(char[20]) __attribute__((__nothrow__));
extern char * tmpnam_r(char __s[20]) __attribute__((__nothrow__));
extern char * tempnam(char const * __dir, char const * __pfx) __attribute__((__nothrow__)) __attribute__((__malloc__));
extern int fflush(FILE * __stream);
extern int fflush_unlocked(FILE * __stream);
extern FILE * fopen(char const * __restrict __filename, char const * __restrict __modes) __attribute__((__malloc__));
extern FILE * freopen(char const * __restrict __filename, char const * __restrict __modes, FILE * __restrict __stream)
    __attribute__((__nonnull__(3)));
extern FILE * fdopen(int __fd, char const * __modes) __attribute__((__nothrow__)) __attribute__((__malloc__));
extern FILE *
fopencookie(void * __restrict __magic_cookie, char const * __restrict __modes, cookie_io_functions_t __io_funcs)
    __attribute__((__nothrow__)) __attribute__((__malloc__));
extern FILE * fmemopen(void * __s, size_t __len, char const * __modes) __attribute__((__nothrow__))
__attribute__((__malloc__));
extern FILE * open_memstream(char ** __bufloc, size_t * __sizeloc) __attribute__((__nothrow__))
__attribute__((__malloc__));
extern void setbuf(FILE * __restrict __stream, char * __restrict __buf) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern int setvbuf(FILE * __restrict __stream, char * __restrict __buf, int __modes, size_t __n)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void setbuffer(FILE * __restrict __stream, char * __restrict __buf, size_t __size) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern void setlinebuf(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int fprintf(FILE * __restrict __stream, char const * __restrict __format, ...) __attribute__((__nonnull__(1)));
extern int printf(char const * __restrict __format, ...);
extern int sprintf(char * __restrict __s, char const * __restrict __format, ...) __attribute__((__nothrow__));
extern int vfprintf(FILE * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg)
    __attribute__((__nonnull__(1)));
extern int vprintf(char const * __restrict __format, __gnuc_va_list __arg);
extern int vsprintf(char * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg)
    __attribute__((__nothrow__));
extern int snprintf(char * __restrict __s, size_t __maxlen, char const * __restrict __format, ...)
    __attribute__((__nothrow__)) __attribute__((__format__(__printf__, 3, 4)));
extern int vsnprintf(char * __restrict __s, size_t __maxlen, char const * __restrict __format, __gnuc_va_list __arg)
    __attribute__((__nothrow__)) __attribute__((__format__(__printf__, 3, 0)));
extern int vasprintf(char ** __restrict __ptr, char const * __restrict __f, __gnuc_va_list __arg)
    __attribute__((__nothrow__)) __attribute__((__format__(__printf__, 2, 0)));
extern int __asprintf(char ** __restrict __ptr, char const * __restrict __fmt, ...) __attribute__((__nothrow__))
__attribute__((__format__(__printf__, 2, 3)));
extern int asprintf(char ** __restrict __ptr, char const * __restrict __fmt, ...) __attribute__((__nothrow__))
__attribute__((__format__(__printf__, 2, 3)));
extern int vdprintf(int __fd, char const * __restrict __fmt, __gnuc_va_list __arg)
    __attribute__((__format__(__printf__, 2, 0)));
extern int dprintf(int __fd, char const * __restrict __fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
extern int fscanf(FILE * __restrict __stream, char const * __restrict __format, ...) __attribute__((__nonnull__(1)));
extern int scanf(char const * __restrict __format, ...);
extern int sscanf(char const * __restrict __s, char const * __restrict __format, ...) __attribute__((__nothrow__));
typedef float _Float32;
typedef double _Float64;
typedef double _Float32x;
typedef long double _Float64x;
extern int fscanf(FILE * __restrict __stream, char const * __restrict __format, ...) __asm__(""
                                                                                             "__isoc99_fscanf")
    __attribute__((__nonnull__(1)));
extern int scanf(char const * __restrict __format, ...) __asm__(""
                                                                "__isoc99_scanf");
extern int sscanf(char const * __restrict __s, char const * __restrict __format, ...) __asm__(""
                                                                                              "__isoc99_sscanf")
    __attribute__((__nothrow__));
extern int vfscanf(FILE * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg)
    __attribute__((__format__(__scanf__, 2, 0))) __attribute__((__nonnull__(1)));
extern int vscanf(char const * __restrict __format, __gnuc_va_list __arg) __attribute__((__format__(__scanf__, 1, 0)));
extern int vsscanf(char const * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg)
    __attribute__((__nothrow__)) __attribute__((__format__(__scanf__, 2, 0)));
extern int
vfscanf(FILE * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg) __asm__(""
                                                                                               "__isoc99_vfscanf")
    __attribute__((__format__(__scanf__, 2, 0))) __attribute__((__nonnull__(1)));
extern int vscanf(char const * __restrict __format, __gnuc_va_list __arg) __asm__(""
                                                                                  "__isoc99_vscanf")
    __attribute__((__format__(__scanf__, 1, 0)));
extern int
vsscanf(char const * __restrict __s, char const * __restrict __format, __gnuc_va_list __arg) __asm__(""
                                                                                                     "__isoc99_vsscanf")
    __attribute__((__nothrow__)) __attribute__((__format__(__scanf__, 2, 0)));
extern int fgetc(FILE * __stream) __attribute__((__nonnull__(1)));
extern int getc(FILE * __stream) __attribute__((__nonnull__(1)));
extern int getchar(void);
extern int getc_unlocked(FILE * __stream) __attribute__((__nonnull__(1)));
extern int getchar_unlocked(void);
extern int fgetc_unlocked(FILE * __stream) __attribute__((__nonnull__(1)));
extern int fputc(int __c, FILE * __stream) __attribute__((__nonnull__(2)));
extern int putc(int __c, FILE * __stream) __attribute__((__nonnull__(2)));
extern int putchar(int __c);
extern int fputc_unlocked(int __c, FILE * __stream) __attribute__((__nonnull__(2)));
extern int putc_unlocked(int __c, FILE * __stream) __attribute__((__nonnull__(2)));
extern int putchar_unlocked(int __c);
extern int getw(FILE * __stream) __attribute__((__nonnull__(1)));
extern int putw(int __w, FILE * __stream) __attribute__((__nonnull__(2)));
extern char * fgets(char * __restrict __s, int __n, FILE * __restrict __stream) __attribute__((__nonnull__(3)));
extern __ssize_t
__getdelim(char ** __restrict __lineptr, size_t * __restrict __n, int __delimiter, FILE * __restrict __stream)
    __attribute__((__nonnull__(4)));
extern __ssize_t
getdelim(char ** __restrict __lineptr, size_t * __restrict __n, int __delimiter, FILE * __restrict __stream)
    __attribute__((__nonnull__(4)));
extern __ssize_t getline(char ** __restrict __lineptr, size_t * __restrict __n, FILE * __restrict __stream)
    __attribute__((__nonnull__(3)));
extern int fputs(char const * __restrict __s, FILE * __restrict __stream) __attribute__((__nonnull__(2)));
extern int puts(char const * __s);
extern int ungetc(int __c, FILE * __stream) __attribute__((__nonnull__(2)));
extern size_t fread(void * __restrict __ptr, size_t __size, size_t __n, FILE * __restrict __stream)
    __attribute__((__nonnull__(4)));
extern size_t fwrite(void const * __restrict __ptr, size_t __size, size_t __n, FILE * __restrict __s)
    __attribute__((__nonnull__(4)));
extern size_t fread_unlocked(void * __restrict __ptr, size_t __size, size_t __n, FILE * __restrict __stream)
    __attribute__((__nonnull__(4)));
extern size_t fwrite_unlocked(void const * __restrict __ptr, size_t __size, size_t __n, FILE * __restrict __stream)
    __attribute__((__nonnull__(4)));
extern int fseek(FILE * __stream, long int __off, int __whence) __attribute__((__nonnull__(1)));
extern long int ftell(FILE * __stream) __attribute__((__nonnull__(1)));
extern void rewind(FILE * __stream) __attribute__((__nonnull__(1)));
extern int fseeko(FILE * __stream, __off_t __off, int __whence) __attribute__((__nonnull__(1)));
extern __off_t ftello(FILE * __stream) __attribute__((__nonnull__(1)));
extern int fgetpos(FILE * __restrict __stream, fpos_t * __restrict __pos) __attribute__((__nonnull__(1)));
extern int fsetpos(FILE * __stream, fpos_t const * __pos) __attribute__((__nonnull__(1)));
extern void clearerr(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int feof(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int ferror(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void clearerr_unlocked(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int feof_unlocked(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int ferror_unlocked(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void perror(char const * __s) __attribute__((__cold__));
extern int fileno(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int fileno_unlocked(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int pclose(FILE * __stream) __attribute__((__nonnull__(1)));
extern FILE * popen(char const * __command, char const * __modes) __attribute__((__malloc__));
extern char * ctermid(char * __s) __attribute__((__nothrow__));
extern void flockfile(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int ftrylockfile(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void funlockfile(FILE * __stream) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int __uflow(FILE *);
extern int __overflow(FILE *, int);
typedef struct epc_parser_t epc_parser_t;
typedef struct epc_cpt_node_t epc_cpt_node_t;
typedef struct epc_parser_ctx_t epc_parser_ctx_t;
typedef struct epc_parser_list epc_parser_list;
typedef struct epc_line_col_t
{
    size_t line;
    size_t col;
} epc_line_col_t;
typedef enum epc_parse_type_t
{
    EPC_PARSE_TYPE_STRING,
    EPC_PARSE_TYPE_FILE,
    EPC_PARSE_TYPE_FILENAME,
    EPC_PARSE_TYPE_FD,
} epc_parse_type_t;
typedef struct epc_parse_input_t
{
    epc_parse_type_t type;
    union
    {
        char const * input_string;
        FILE * fp;
        char const * filename;
        int fd;
    };
} epc_parse_input_t;
typedef struct epc_parser_error_t
{
    char const * message;
    char const * input_position;
    epc_line_col_t position;
    char const * expected;
    char const * found;
} epc_parser_error_t;
typedef struct
{
    _Bool assigned;
    int action;
} epc_ast_semantic_action_t;
typedef struct
{
    _Bool is_error;
    union
    {
        epc_cpt_node_t * success;
        epc_parser_error_t * error;
    } data;
} epc_parse_result_t;
typedef struct epc_parse_session_t
{
    epc_parse_result_t result;
    epc_parser_ctx_t * internal_parse_ctx;
} epc_parse_session_t;
typedef struct
{
    void (*enter_node)(epc_cpt_node_t * node, void * user_data);
    void (*exit_node)(epc_cpt_node_t * node, void * user_data);
    void * user_data;
} epc_cpt_visitor_t;
__attribute__((visibility("default"))) void epc_cpt_visit_nodes(epc_cpt_node_t * root, epc_cpt_visitor_t * visitor);
__attribute__((visibility("default"))) epc_parser_list * epc_parser_list_create(void);
__attribute__((visibility("default"))) epc_parser_t *
epc_parser_list_add(epc_parser_list * list, epc_parser_t * parser);
__attribute__((visibility("default"))) void epc_parser_list_free(epc_parser_list * list);
__attribute__((visibility("default"))) epc_parser_t * epc_char(char const * name, char c);
static inline epc_parser_t *
epc_char_l(epc_parser_list * list, char const * name, char c)
{
    return epc_parser_list_add(list, epc_char(name, c));
}
__attribute__((visibility("default"))) epc_parser_t * epc_string(char const * name, char const * s);
static inline epc_parser_t *
epc_string_l(epc_parser_list * list, char const * name, char const * s)
{
    return epc_parser_list_add(list, epc_string(name, s));
}
__attribute__((visibility("default"))) epc_parser_t * epc_digit(char const * name);
static inline epc_parser_t *
epc_digit_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_digit(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_alpha(char const * name);
static inline epc_parser_t *
epc_alpha_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_alpha(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_alphanum(char const * name);
static inline epc_parser_t *
epc_alphanum_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_alphanum(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_int(char const * name);
static inline epc_parser_t *
epc_int_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_int(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_double(char const * name);
static inline epc_parser_t *
epc_double_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_double(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_space(char const * name);
static inline epc_parser_t *
epc_space_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_space(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_any(char const * name);
static inline epc_parser_t *
epc_any_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_any(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_identifier(char const * name);
static inline epc_parser_t *
epc_identifier_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_identifier(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_octal(char const * name);
static inline epc_parser_t *
epc_octal_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_octal(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_hex(char const * name);
static inline epc_parser_t *
epc_hex_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_hex(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_succeed(char const * name);
static inline epc_parser_t *
epc_succeed_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_succeed(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_hex_digit(char const * name);
static inline epc_parser_t *
epc_hex_digit_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_hex_digit(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_char_range(char const * name, char char_start, char char_end);
static inline epc_parser_t *
epc_char_range_l(epc_parser_list * list, char const * name, char char_start, char char_end)
{
    return epc_parser_list_add(list, epc_char_range(name, char_start, char_end));
}
__attribute__((visibility("default"))) epc_parser_t * epc_none_of(char const * name, char const * chars_to_avoid);
static inline epc_parser_t *
epc_none_of_l(epc_parser_list * list, char const * name, char const * chars_to_avoid)
{
    return epc_parser_list_add(list, epc_none_of(name, chars_to_avoid));
}
__attribute__((visibility("default"))) epc_parser_t * epc_many(char const * name, epc_parser_t * p);
static inline epc_parser_t *
epc_many_l(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_many(name, p));
}
__attribute__((visibility("default"))) epc_parser_t * epc_count(char const * name, int num, epc_parser_t * p);
static inline epc_parser_t *
epc_count_l(epc_parser_list * list, char const * name, int num, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_count(name, num, p));
}
__attribute__((visibility("default"))) epc_parser_t *
epc_between(char const * name, epc_parser_t * open, epc_parser_t * p, epc_parser_t * close);
static inline epc_parser_t *
epc_between_l(epc_parser_list * list, char const * name, epc_parser_t * open, epc_parser_t * p, epc_parser_t * close)
{
    return epc_parser_list_add(list, epc_between(name, open, p, close));
}
__attribute__((visibility("default"))) epc_parser_t *
epc_delimited(char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser);
__attribute__((visibility("default"))) epc_parser_t *
epc_delimited_flex(char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser);
static inline epc_parser_t *
epc_delimited_l(epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser)
{
    return epc_parser_list_add(list, epc_delimited(name, item_parser, delimiter_parser));
}
static inline epc_parser_t *
epc_delimited_flex_l(
    epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser
)
{
    return epc_parser_list_add(list, epc_delimited_flex(name, item_parser, delimiter_parser));
}
__attribute__((visibility("default"))) epc_parser_t * epc_optional(char const * name, epc_parser_t * p);
static inline epc_parser_t *
epc_optional_l(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_optional(name, p));
}
__attribute__((visibility("default"))) epc_parser_t * epc_lookahead(char const * name, epc_parser_t * p);
static inline epc_parser_t *
epc_lookahead_l(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_lookahead(name, p));
}
__attribute__((visibility("default"))) epc_parser_t * epc_not(char const * name, epc_parser_t * p);
static inline epc_parser_t *
epc_not_l(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_not(name, p));
}
__attribute__((visibility("default"))) epc_parser_t * epc_fail(char const * name, char const * message);
static inline epc_parser_t *
epc_fail_l(epc_parser_list * list, char const * name, char const * message)
{
    return epc_parser_list_add(list, epc_fail(name, message));
}
__attribute__((visibility("default"))) epc_parser_t * epc_one_of(char const * name, char const * chars_to_match);
static inline epc_parser_t *
epc_one_of_l(epc_parser_list * list, char const * name, char const * chars_to_match)
{
    return epc_parser_list_add(list, epc_one_of(name, chars_to_match));
}
__attribute__((visibility("default"))) epc_parser_t * epc_lexeme(char const * name, epc_parser_t * p);
static inline epc_parser_t *
epc_lexeme_l(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, epc_lexeme(name, p));
}
__attribute__((visibility("default"))) epc_parser_t *
epc_chainl1(char const * name, epc_parser_t * item, epc_parser_t * op);
static inline epc_parser_t *
epc_chainl1_l(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op)
{
    return epc_parser_list_add(list, epc_chainl1(name, item, op));
}
__attribute__((visibility("default"))) epc_parser_t *
epc_chainr1(char const * name, epc_parser_t * item, epc_parser_t * op);
static inline epc_parser_t *
epc_chainr1_l(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op)
{
    return epc_parser_list_add(list, epc_chainr1(name, item, op));
}
__attribute__((visibility("default"))) epc_parser_t * epc_or(char const * name, int count, ...);
__attribute__((visibility("default"))) epc_parser_t *
epc_or_l(epc_parser_list * list, char const * name, int count, ...);
__attribute__((visibility("default"))) epc_parser_t * epc_and(char const * name, int count, ...);
__attribute__((visibility("default"))) epc_parser_t *
epc_and_l(epc_parser_list * list, char const * name, int count, ...);
__attribute__((visibility("default"))) epc_parser_t * epc_skip(char const * name, epc_parser_t * parser_to_skip);
static inline epc_parser_t *
epc_skip_l(epc_parser_list * list, char const * name, epc_parser_t * parser_to_skip)
{
    return epc_parser_list_add(list, epc_skip(name, parser_to_skip));
}
__attribute__((visibility("default"))) epc_parser_t * epc_plus(char const * name, epc_parser_t * parser_to_repeat);
static inline epc_parser_t *
epc_plus_l(epc_parser_list * list, char const * name, epc_parser_t * parser_to_repeat)
{
    return epc_parser_list_add(list, epc_plus(name, parser_to_repeat));
}
__attribute__((visibility("default"))) epc_parser_t * epc_eoi(char const * name);
static inline epc_parser_t *
epc_eoi_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_eoi(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_cpp_comment(char const * name);
static inline epc_parser_t *
epc_cpp_comment_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_cpp_comment(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_bash_comment(char const * name);
static inline epc_parser_t *
epc_bash_comment_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_bash_comment(name));
}
__attribute__((visibility("default"))) epc_parser_t * epc_c_comment(char const * name);
static inline epc_parser_t *
epc_c_comment_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_c_comment(name));
}
typedef _Bool (*epc_satisfy_parser_predicate_fn)(epc_cpt_node_t * token, epc_parser_ctx_t * parse_ctx, void * user_ctx);
__attribute__((visibility("default"))) epc_parser_t * epc_satisfy(
    char const * name,
    epc_parser_t * token_parser,
    char const * message,
    epc_satisfy_parser_predicate_fn predicate,
    void * parser_data
);
static inline epc_parser_t *
epc_satisfy_l(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * token_parser,
    char const * message,
    epc_satisfy_parser_predicate_fn predicate,
    void * parser_data
)
{
    return epc_parser_list_add(list, epc_satisfy(name, token_parser, message, predicate, parser_data));
}
typedef void (*epc_wrap_entry_fn)(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data);
typedef _Bool (*epc_wrap_exit_fn)(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data);
typedef struct epc_wrap_callbacks
{
    epc_wrap_entry_fn on_entry;
    epc_wrap_exit_fn on_exit;
} epc_wrap_callbacks_t;
__attribute__((visibility("default"))) epc_parser_t *
epc_wrap(char const * name, epc_parser_t * wrapped_parser, epc_wrap_callbacks_t callbacks, void * parser_data);
static inline epc_parser_t *
epc_wrap_l(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * wrapped_parser,
    epc_wrap_callbacks_t callbacks,
    void * parser_data
)
{
    return epc_parser_list_add(list, epc_wrap(name, wrapped_parser, callbacks, parser_data));
}
__attribute__((visibility("default"))) epc_parser_t * epc_parser_fwd_decl(char const * name);
static inline epc_parser_t *
epc_parser_fwd_decl_l(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, epc_parser_fwd_decl(name));
}
__attribute__((visibility("default"))) void epc_parser_duplicate(epc_parser_t * dst, epc_parser_t const * src);
__attribute__((visibility("default"))) void epc_parser_set_ast_action(epc_parser_t * p, int action_type);
__attribute__((visibility("default"))) void * parse_ctx_get_user_ctx(epc_parser_ctx_t const * ctx);
__attribute__((visibility("default"))) epc_parse_session_t
epc_parse_str(epc_parser_t * top_parser, char const * input_string, void * user_ctx);
__attribute__((visibility("default"))) epc_parse_session_t
epc_parse_fp(epc_parser_t * top_parser, FILE * fp, void * user_ctx);
__attribute__((visibility("default"))) epc_parse_session_t
epc_parse_file(epc_parser_t * top_parser, char const * filename, void * user_ctx);
__attribute__((visibility("default"))) epc_parse_session_t
epc_parse_fd(epc_parser_t * top_parser, int fd, void * user_ctx);
__attribute__((visibility("default"))) void epc_parse_session_destroy(epc_parse_session_t * session);
__attribute__((visibility("default"))) void epc_parse_session_print_cpt(FILE * fp, epc_parse_session_t const * session);
__attribute__((visibility("default"))) const char * epc_cpt_node_get_semantic_content(epc_cpt_node_t * node);
__attribute__((visibility("default"))) size_t epc_cpt_node_get_semantic_len(epc_cpt_node_t * node);
__attribute__((visibility("default"))) const char * epc_cpt_node_get_content(epc_cpt_node_t * node);
__attribute__((visibility("default"))) size_t epc_cpt_node_get_content_len(epc_cpt_node_t * node);
__attribute__((visibility("default"))) char * epc_cpt_to_string(epc_parser_ctx_t * parse_ctx, epc_cpt_node_t * node);
void epc_parsers_free(size_t const count, ...);
__attribute__((visibility("default"))) char const * epc_get_version(void);
typedef enum
{
    C_GRAMMAR_AST_ACTION_COUNT__,
} c_grammar_semantic_action_t;

epc_parser_t * create_c_grammar_parser(epc_parser_list * list);
_Bool is_typedef_name(epc_cpt_node_t * token, epc_parser_ctx_t * parse_ctx, void * parser_data);
extern epc_wrap_callbacks_t typedef_capture_callbacks;
extern epc_wrap_callbacks_t typedef_commit_callbacks;
typedef struct
{
    int quot;
    int rem;
} div_t;
typedef struct
{
    long int quot;
    long int rem;
} ldiv_t;
__extension__ typedef struct
{
    long long int quot;
    long long int rem;
} lldiv_t;
extern size_t __ctype_get_mb_cur_max(void) __attribute__((__nothrow__));
extern double atof(char const * __nptr) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern int atoi(char const * __nptr) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern long int atol(char const * __nptr) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
__extension__ extern long long int atoll(char const * __nptr) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern double strtod(char const * __restrict __nptr, char ** __restrict __endptr) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern float strtof(char const * __restrict __nptr, char ** __restrict __endptr) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern long double strtold(char const * __restrict __nptr, char ** __restrict __endptr) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern long int strtol(char const * __restrict __nptr, char ** __restrict __endptr, int __base)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern unsigned long int strtoul(char const * __restrict __nptr, char ** __restrict __endptr, int __base)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
__extension__ extern long long int strtoq(char const * __restrict __nptr, char ** __restrict __endptr, int __base)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
__extension__ extern unsigned long long int
strtouq(char const * __restrict __nptr, char ** __restrict __endptr, int __base) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
__extension__ extern long long int strtoll(char const * __restrict __nptr, char ** __restrict __endptr, int __base)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
__extension__ extern unsigned long long int
strtoull(char const * __restrict __nptr, char ** __restrict __endptr, int __base) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern char * l64a(long int __n) __attribute__((__nothrow__));
extern long int a64l(char const * __s) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __loff_t loff_t;
typedef __ino_t ino_t;
typedef __dev_t dev_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;
typedef __key_t key_t;
typedef __clock_t clock_t;

typedef __clockid_t clockid_t;
typedef __time_t time_t;
typedef __timer_t timer_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;
typedef int register_t __attribute__((__mode__(__word__)));
static __inline __uint16_t
__bswap_16(__uint16_t __bsx)
{
    return ((__uint16_t)((((__bsx) >> 8) & 0xff) | (((__bsx) & 0xff) << 8)));
}
static __inline __uint32_t
__bswap_32(__uint32_t __bsx)
{
    return (
        (((__bsx) & 0xff000000u) >> 24) | (((__bsx) & 0x00ff0000u) >> 8) | (((__bsx) & 0x0000ff00u) << 8)
        | (((__bsx) & 0x000000ffu) << 24)
    );
}
__extension__ static __inline __uint64_t
__bswap_64(__uint64_t __bsx)
{
    return (
        (((__bsx) & 0xff00000000000000ull) >> 56) | (((__bsx) & 0x00ff000000000000ull) >> 40)
        | (((__bsx) & 0x0000ff0000000000ull) >> 24) | (((__bsx) & 0x000000ff00000000ull) >> 8)
        | (((__bsx) & 0x00000000ff000000ull) << 8) | (((__bsx) & 0x0000000000ff0000ull) << 24)
        | (((__bsx) & 0x000000000000ff00ull) << 40) | (((__bsx) & 0x00000000000000ffull) << 56)
    );
}
static __inline __uint16_t
__uint16_identity(__uint16_t __x)
{
    return __x;
}
static __inline __uint32_t
__uint32_identity(__uint32_t __x)
{
    return __x;
}
static __inline __uint64_t
__uint64_identity(__uint64_t __x)
{
    return __x;
}
typedef struct
{
    unsigned long int __val[(1024 / (8 * sizeof(unsigned long int)))];
} __sigset_t;
typedef __sigset_t sigset_t;
struct timeval
{
    __time_t tv_sec;
    __suseconds_t tv_usec;
};

struct timespec
{
    __time_t tv_sec;
    __syscall_slong_t tv_nsec;
};
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
typedef struct
{
    __fd_mask __fds_bits[1024 / (8 * (int)sizeof(__fd_mask))];
} fd_set;
typedef __fd_mask fd_mask;
extern int select(
    int __nfds,
    fd_set * __restrict __readfds,
    fd_set * __restrict __writefds,
    fd_set * __restrict __exceptfds,
    struct timeval * __restrict __timeout
);
extern int pselect(
    int __nfds,
    fd_set * __restrict __readfds,
    fd_set * __restrict __writefds,
    fd_set * __restrict __exceptfds,
    const struct timespec * __restrict __timeout,
    __sigset_t const * __restrict __sigmask
);
typedef __blksize_t blksize_t;
typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;

typedef union
{
    __extension__ unsigned long long int __value64;
    struct
    {
        unsigned int __low;
        unsigned int __high;
    } __value32;
} __atomic_wide_counter;
typedef struct __pthread_internal_list
{
    struct __pthread_internal_list * __prev;
    struct __pthread_internal_list * __next;
} __pthread_list_t;
typedef struct __pthread_internal_slist
{
    struct __pthread_internal_slist * __next;
} __pthread_slist_t;
struct __pthread_mutex_s
{
    int __lock;
    unsigned int __count;
    int __owner;
    unsigned int __nusers;
    int __kind;
    short __spins;
    short __elision;
    __pthread_list_t __list;
};
struct __pthread_rwlock_arch_t
{
    unsigned int __readers;
    unsigned int __writers;
    unsigned int __wrphase_futex;
    unsigned int __writers_futex;
    unsigned int __pad3;
    unsigned int __pad4;
    int __cur_writer;
    int __shared;
    signed char __rwelision;
    unsigned char __pad1[7];
    unsigned long int __pad2;
    unsigned int __flags;
};
struct __pthread_cond_s
{
    __atomic_wide_counter __wseq;
    __atomic_wide_counter __g1_start;
    unsigned int __g_refs[2];
    unsigned int __g_size[2];
    unsigned int __g1_orig_size;
    unsigned int __wrefs;
    unsigned int __g_signals[2];
};
typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;
typedef struct
{
    int __data;
} __once_flag;
typedef unsigned long int pthread_t;
typedef union
{
    char __size[4];
    int __align;
} pthread_mutexattr_t;
typedef union
{
    char __size[4];
    int __align;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
union pthread_attr_t
{
    char __size[56];
    long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef union
{
    struct __pthread_mutex_s __data;
    char __size[40];
    long int __align;
} pthread_mutex_t;
typedef union
{
    struct __pthread_cond_s __data;
    char __size[48];
    __extension__ long long int __align;
} pthread_cond_t;
typedef union
{
    struct __pthread_rwlock_arch_t __data;
    char __size[56];
    long int __align;
} pthread_rwlock_t;
typedef union
{
    char __size[8];
    long int __align;
} pthread_rwlockattr_t;
typedef int volatile pthread_spinlock_t;
typedef union
{
    char __size[32];
    long int __align;
} pthread_barrier_t;
typedef union
{
    char __size[4];
    int __align;
} pthread_barrierattr_t;
extern long int random(void) __attribute__((__nothrow__));
extern void srandom(unsigned int __seed) __attribute__((__nothrow__));
extern char * initstate(unsigned int __seed, char * __statebuf, size_t __statelen) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
extern char * setstate(char * __statebuf) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
struct random_data
{
    int32_t * fptr;
    int32_t * rptr;
    int32_t * state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t * end_ptr;
};
extern int random_r(struct random_data * __restrict __buf, int32_t * __restrict __result) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern int srandom_r(unsigned int __seed, struct random_data * __buf) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
extern int
initstate_r(unsigned int __seed, char * __restrict __statebuf, size_t __statelen, struct random_data * __restrict __buf)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(2, 4)));
extern int setstate_r(char * __restrict __statebuf, struct random_data * __restrict __buf) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern int rand(void) __attribute__((__nothrow__));
extern void srand(unsigned int __seed) __attribute__((__nothrow__));
extern int rand_r(unsigned int * __seed) __attribute__((__nothrow__));
extern double drand48(void) __attribute__((__nothrow__));
extern double erand48(unsigned short int __xsubi[3]) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern long int lrand48(void) __attribute__((__nothrow__));
extern long int nrand48(unsigned short int __xsubi[3]) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern long int mrand48(void) __attribute__((__nothrow__));
extern long int jrand48(unsigned short int __xsubi[3]) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void srand48(long int __seedval) __attribute__((__nothrow__));
extern unsigned short int * seed48(unsigned short int __seed16v[3]) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern void lcong48(unsigned short int __param[7]) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
struct drand48_data
{
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;
};
extern int drand48_r(struct drand48_data * __restrict __buffer, double * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int
erand48_r(unsigned short int __xsubi[3], struct drand48_data * __restrict __buffer, double * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int lrand48_r(struct drand48_data * __restrict __buffer, long int * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int
nrand48_r(unsigned short int __xsubi[3], struct drand48_data * __restrict __buffer, long int * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int mrand48_r(struct drand48_data * __restrict __buffer, long int * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int
jrand48_r(unsigned short int __xsubi[3], struct drand48_data * __restrict __buffer, long int * __restrict __result)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern int srand48_r(long int __seedval, struct drand48_data * __buffer) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
extern int seed48_r(unsigned short int __seed16v[3], struct drand48_data * __buffer) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern int lcong48_r(unsigned short int __param[7], struct drand48_data * __buffer) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern __uint32_t arc4random(void) __attribute__((__nothrow__));
extern void arc4random_buf(void * __buf, size_t __size) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern __uint32_t arc4random_uniform(__uint32_t __upper_bound) __attribute__((__nothrow__));
extern void * malloc(size_t __size) __attribute__((__nothrow__)) __attribute__((__malloc__));
extern void * calloc(size_t __nmemb, size_t __size) __attribute__((__nothrow__)) __attribute__((__malloc__));
extern void * realloc(void * __ptr, size_t __size) __attribute__((__nothrow__)) __attribute__((__warn_unused_result__));
extern void free(void * __ptr) __attribute__((__nothrow__));
extern void * reallocarray(void * __ptr, size_t __nmemb, size_t __size) __attribute__((__nothrow__))
__attribute__((__warn_unused_result__));
extern void * reallocarray(void * __ptr, size_t __nmemb, size_t __size) __attribute__((__nothrow__));
extern void * alloca(size_t __size) __attribute__((__nothrow__));
extern void * valloc(size_t __size) __attribute__((__nothrow__)) __attribute__((__malloc__));
extern int posix_memalign(void ** __memptr, size_t __alignment, size_t __size) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern void * aligned_alloc(size_t __alignment, size_t __size) __attribute__((__nothrow__)) __attribute__((__malloc__))
__attribute__((__alloc_align__(1)));
extern void abort(void) __attribute__((__nothrow__)) __attribute__((__noreturn__));
extern int atexit(void (*__func)(void)) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int at_quick_exit(void (*__func)(void)) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int on_exit(void (*__func)(int __status, void * __arg), void * __arg) __attribute__((__nothrow__))
__attribute__((__nonnull__(1)));
extern void exit(int __status) __attribute__((__nothrow__)) __attribute__((__noreturn__));
extern void quick_exit(int __status) __attribute__((__nothrow__)) __attribute__((__noreturn__));
extern void _Exit(int __status) __attribute__((__nothrow__)) __attribute__((__noreturn__));
extern char * getenv(char const * __name) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int putenv(char * __string) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int setenv(char const * __name, char const * __value, int __replace) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
extern int unsetenv(char const * __name) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int clearenv(void) __attribute__((__nothrow__));
extern char * mktemp(char * __template) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int mkstemp(char * __template) __attribute__((__nonnull__(1)));
extern int mkstemps(char * __template, int __suffixlen) __attribute__((__nonnull__(1)));
extern char * mkdtemp(char * __template) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int system(char const * __command);
extern char * realpath(char const * __restrict __name, char * __restrict __resolved) __attribute__((__nothrow__));
typedef int (*__compar_fn_t)(void const *, void const *);
extern void * bsearch(void const * __key, void const * __base, size_t __nmemb, size_t __size, __compar_fn_t __compar)
    __attribute__((__nonnull__(1, 2, 5)));
extern void qsort(void * __base, size_t __nmemb, size_t __size, __compar_fn_t __compar)
    __attribute__((__nonnull__(1, 4)));
extern int abs(int __x) __attribute__((__nothrow__)) __attribute__((__const__));
extern long int labs(long int __x) __attribute__((__nothrow__)) __attribute__((__const__));
__extension__ extern long long int llabs(long long int __x) __attribute__((__nothrow__)) __attribute__((__const__));
extern div_t div(int __numer, int __denom) __attribute__((__nothrow__)) __attribute__((__const__));
extern ldiv_t ldiv(long int __numer, long int __denom) __attribute__((__nothrow__)) __attribute__((__const__));
__extension__ extern lldiv_t lldiv(long long int __numer, long long int __denom) __attribute__((__nothrow__))
__attribute__((__const__));
extern char * ecvt(double __value, int __ndigit, int * __restrict __decpt, int * __restrict __sign)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4)));
extern char * fcvt(double __value, int __ndigit, int * __restrict __decpt, int * __restrict __sign)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4)));
extern char * gcvt(double __value, int __ndigit, char * __buf) __attribute__((__nothrow__))
__attribute__((__nonnull__(3)));
extern char * qecvt(long double __value, int __ndigit, int * __restrict __decpt, int * __restrict __sign)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4)));
extern char * qfcvt(long double __value, int __ndigit, int * __restrict __decpt, int * __restrict __sign)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4)));
extern char * qgcvt(long double __value, int __ndigit, char * __buf) __attribute__((__nothrow__))
__attribute__((__nonnull__(3)));
extern int ecvt_r(
    double __value,
    int __ndigit,
    int * __restrict __decpt,
    int * __restrict __sign,
    char * __restrict __buf,
    size_t __len
) __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4, 5)));
extern int fcvt_r(
    double __value,
    int __ndigit,
    int * __restrict __decpt,
    int * __restrict __sign,
    char * __restrict __buf,
    size_t __len
) __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4, 5)));
extern int qecvt_r(
    long double __value,
    int __ndigit,
    int * __restrict __decpt,
    int * __restrict __sign,
    char * __restrict __buf,
    size_t __len
) __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4, 5)));
extern int qfcvt_r(
    long double __value,
    int __ndigit,
    int * __restrict __decpt,
    int * __restrict __sign,
    char * __restrict __buf,
    size_t __len
) __attribute__((__nothrow__)) __attribute__((__nonnull__(3, 4, 5)));
extern int mblen(char const * __s, size_t __n) __attribute__((__nothrow__));
extern int mbtowc(wchar_t * __restrict __pwc, char const * __restrict __s, size_t __n) __attribute__((__nothrow__));
extern int wctomb(char * __s, wchar_t __wchar) __attribute__((__nothrow__));
extern size_t mbstowcs(wchar_t * __restrict __pwcs, char const * __restrict __s, size_t __n)
    __attribute__((__nothrow__));
extern size_t wcstombs(char * __restrict __s, wchar_t const * __restrict __pwcs, size_t __n)
    __attribute__((__nothrow__));
extern int rpmatch(char const * __response) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int getsubopt(char ** __restrict __optionp, char * const * __restrict __tokens, char ** __restrict __valuep)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2, 3)));
extern int getloadavg(double __loadavg[], int __nelem) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern void * memcpy(void * __restrict __dest, void const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern void * memmove(void * __dest, void const * __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern void * memccpy(void * __restrict __dest, void const * __restrict __src, int __c, size_t __n)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern void * memset(void * __s, int __c, size_t __n) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern int memcmp(void const * __s1, void const * __s2, size_t __n) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern int __memcmpeq(void const * __s1, void const * __s2, size_t __n) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern void * memchr(void const * __s, int __c, size_t __n) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern char * strcpy(char * __restrict __dest, char const * __restrict __src) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * strncpy(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * strcat(char * __restrict __dest, char const * __restrict __src) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * strncat(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern int strcmp(char const * __s1, char const * __s2) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern int strncmp(char const * __s1, char const * __s2, size_t __n) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern int strcoll(char const * __s1, char const * __s2) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern size_t strxfrm(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
struct __locale_struct
{
    struct __locale_data * __locales[13];
    unsigned short int const * __ctype_b;
    int const * __ctype_tolower;
    int const * __ctype_toupper;
    char const * __names[13];
};
typedef struct __locale_struct * __locale_t;

typedef __locale_t locale_t;
extern int strcoll_l(char const * __s1, char const * __s2, locale_t __l) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2, 3)));
extern size_t strxfrm_l(char * __dest, char const * __src, size_t __n, locale_t __l) __attribute__((__nothrow__))
__attribute__((__nonnull__(2, 4)));
extern char * strdup(char const * __s) __attribute__((__nothrow__)) __attribute__((__malloc__))
__attribute__((__nonnull__(1)));
extern char * strndup(char const * __string, size_t __n) __attribute__((__nothrow__)) __attribute__((__malloc__))
__attribute__((__nonnull__(1)));
extern char * strchr(char const * __s, int __c) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern char * strrchr(char const * __s, int __c) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern char * strchrnul(char const * __s, int __c) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern size_t strcspn(char const * __s, char const * __reject) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern size_t strspn(char const * __s, char const * __accept) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern char * strpbrk(char const * __s, char const * __accept) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern char * strstr(char const * __haystack, char const * __needle) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern char * strtok(char * __restrict __s, char const * __restrict __delim) __attribute__((__nothrow__))
__attribute__((__nonnull__(2)));
extern char * __strtok_r(char * __restrict __s, char const * __restrict __delim, char ** __restrict __save_ptr)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(2, 3)));
extern char * strtok_r(char * __restrict __s, char const * __restrict __delim, char ** __restrict __save_ptr)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(2, 3)));
extern char * strcasestr(char const * __haystack, char const * __needle) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern void * memmem(void const * __haystack, size_t __haystacklen, void const * __needle, size_t __needlelen)
    __attribute__((__nothrow__)) __attribute__((__pure__)) __attribute__((__nonnull__(1, 3)));
extern void * __mempcpy(void * __restrict __dest, void const * __restrict __src, size_t __n)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern void * mempcpy(void * __restrict __dest, void const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern size_t strlen(char const * __s) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern size_t strnlen(char const * __string, size_t __maxlen) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern char * strerror(int __errnum) __attribute__((__nothrow__));
extern int strerror_r(int __errnum, char * __buf, size_t __buflen) __asm__(""
                                                                           "__xpg_strerror_r")
    __attribute__((__nothrow__)) __attribute__((__nonnull__(2)));
extern char * strerror_l(int __errnum, locale_t __l) __attribute__((__nothrow__));
extern int bcmp(void const * __s1, void const * __s2, size_t __n) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern void bcopy(void const * __src, void * __dest, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern void bzero(void * __s, size_t __n) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern char * index(char const * __s, int __c) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern char * rindex(char const * __s, int __c) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1)));
extern int ffs(int __i) __attribute__((__nothrow__)) __attribute__((__const__));
extern int ffsl(long int __l) __attribute__((__nothrow__)) __attribute__((__const__));
__extension__ extern int ffsll(long long int __ll) __attribute__((__nothrow__)) __attribute__((__const__));
extern int strcasecmp(char const * __s1, char const * __s2) __attribute__((__nothrow__)) __attribute__((__pure__))
__attribute__((__nonnull__(1, 2)));
extern int strncasecmp(char const * __s1, char const * __s2, size_t __n) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2)));
extern int strcasecmp_l(char const * __s1, char const * __s2, locale_t __loc) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2, 3)));
extern int strncasecmp_l(char const * __s1, char const * __s2, size_t __n, locale_t __loc) __attribute__((__nothrow__))
__attribute__((__pure__)) __attribute__((__nonnull__(1, 2, 4)));
extern void explicit_bzero(void * __s, size_t __n) __attribute__((__nothrow__)) __attribute__((__nonnull__(1)));
extern char * strsep(char ** __restrict __stringp, char const * __restrict __delim) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * strsignal(int __sig) __attribute__((__nothrow__));
extern char * __stpcpy(char * __restrict __dest, char const * __restrict __src) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * stpcpy(char * __restrict __dest, char const * __restrict __src) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern char * __stpncpy(char * __restrict __dest, char const * __restrict __src, size_t __n)
    __attribute__((__nothrow__)) __attribute__((__nonnull__(1, 2)));
extern char * stpncpy(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern size_t strlcpy(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
extern size_t strlcat(char * __restrict __dest, char const * __restrict __src, size_t __n) __attribute__((__nothrow__))
__attribute__((__nonnull__(1, 2)));
typedef struct
{
    char * names[1024];
    int count;
} symbol_table_t;
symbol_table_t *
symbol_table_create()
{
    return calloc(1, sizeof(symbol_table_t));
}
void
symbol_table_free(symbol_table_t * st)
{
    if (!st)
        return;
    for (int i = 0; i < st->count; i++)
    {
        free(st->names[i]);
    }
    free(st);
}
void
symbol_table_add(symbol_table_t * st, char const * name)
{
    if (st->count >= 1024)
        return;
    for (int i = 0; i < st->count; i++)
    {
        if (strcmp(st->names[i], name) == 0)
            return;
    }
    st->names[st->count++] = strdup(name);
}
_Bool
symbol_table_contains(symbol_table_t * st, char const * name)
{
    for (int i = 0; i < st->count; i++)
    {
        if (strcmp(st->names[i], name) == 0)
            return 1;
    }
    return 0;
}
typedef struct
{
    symbol_table_t * symbols;
    char ** pending;
    int pending_count;
    int pending_capacity;
    int * marker_stack;
    int marker_top;
    int marker_capacity;
} parse_session_ctx_t;
parse_session_ctx_t *
session_ctx_create()
{
    parse_session_ctx_t * ctx = calloc(1, sizeof(parse_session_ctx_t));
    ctx->symbols = symbol_table_create();
    ctx->pending_capacity = 16;
    ctx->pending = malloc(sizeof(char *) * ctx->pending_capacity);
    ctx->marker_capacity = 16;
    ctx->marker_stack = malloc(sizeof(int) * ctx->marker_capacity);
    return ctx;
}
void
session_ctx_free(parse_session_ctx_t * ctx)
{
    if (!ctx)
        return;
    symbol_table_free(ctx->symbols);
    for (int i = 0; i < ctx->pending_count; i++)
    {
        free(ctx->pending[i]);
    }
    free(ctx->pending);
    free(ctx->marker_stack);
    free(ctx);
}
void
session_ctx_push_pending(parse_session_ctx_t * ctx, char const * name)
{
    if (ctx->pending_count >= ctx->pending_capacity)
    {
        ctx->pending_capacity *= 2;
        ctx->pending = realloc(ctx->pending, sizeof(char *) * ctx->pending_capacity);
    }
    ctx->pending[ctx->pending_count++] = strdup(name);
}
_Bool
is_typedef_name(epc_cpt_node_t * token, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return 0;
    char const * name = epc_cpt_node_get_semantic_content(token);
    size_t len = epc_cpt_node_get_semantic_len(token);
    char * name_copy = strndup(name, len);
    _Bool found = symbol_table_contains(session->symbols, name_copy);
    free(name_copy);
    return found;
}
static void
on_capture_entry(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser;
    (void)parse_ctx;
    (void)parser_data;
}
static _Bool
on_capture_exit(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    if (result.is_error)
        return 1;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return 1;
    char const * name = epc_cpt_node_get_semantic_content(result.data.success);
    size_t len = epc_cpt_node_get_semantic_len(result.data.success);
    char * name_copy = strndup(name, len);
    session_ctx_push_pending(session, name_copy);
    free(name_copy);
    return 1;
}
static void
on_commit_entry(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser;
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return;
    if (session->marker_top >= session->marker_capacity)
    {
        session->marker_capacity *= 2;
        session->marker_stack = realloc(session->marker_stack, sizeof(int) * session->marker_capacity);
    }
    session->marker_stack[session->marker_top++] = session->pending_count;
}
static _Bool
on_commit_exit(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session || session->marker_top == 0)
        return 1;
    int marker = session->marker_stack[--session->marker_top];
    if (!result.is_error)
    {
        for (int i = marker; i < session->pending_count; i++)
        {
            symbol_table_add(session->symbols, session->pending[i]);
            printf("Committed typedef: '%s'\n", session->pending[i]);
            free(session->pending[i]);
        }
        session->pending_count = marker;
    }
    else
    {
        for (int i = marker; i < session->pending_count; i++)
        {
            free(session->pending[i]);
        }
        session->pending_count = marker;
    }
    return 1;
}
epc_wrap_callbacks_t typedef_capture_callbacks = {on_capture_entry, on_capture_exit};
epc_wrap_callbacks_t typedef_commit_callbacks = {on_commit_entry, on_commit_exit};
int
main(int argc, char * argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    char const * filename = argv[1];
    printf("Attempting to parse C file: %s\n", filename);
    epc_parser_list * list = epc_parser_list_create();
    if (!list)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        return 1;
    }
    parse_session_ctx_t * session_ctx = session_ctx_create();
    epc_parser_t * c_parser = create_c_grammar_parser(list);
    if (!c_parser)
    {
        fprintf(stderr, "Failed to create C parser.\n");
        session_ctx_free(session_ctx);
        epc_parser_list_free(list);
        return 1;
    }
    epc_parse_session_t session = epc_parse_file(c_parser, filename, session_ctx);
    if (session.result.is_error)
    {
        epc_parser_error_t * err = session.result.data.error;
        fprintf(stderr, "Parse Error: %s\n", err->message);
        fprintf(stderr, "At line %zu, col %zu\n", err->position.line + 1, err->position.col + 1);
        fprintf(stderr, "Expected: %s\n", err->expected ? err->expected : "unknown");
        fprintf(stderr, "Found: %s\n", err->found ? err->found : "unknown");
        epc_parse_session_destroy(&session);
        session_ctx_free(session_ctx);
        epc_parser_list_free(list);
        return 1;
    }
    printf("Successfully parsed the C file!\n");
    char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
    if (cpt_str)
    {
        printf("Concrete Parse Tree:\n%s\n", cpt_str);
        free(cpt_str);
    }
    epc_parse_session_destroy(&session);
    session_ctx_free(session_ctx);
    epc_parser_list_free(list);
    return 0;
}
