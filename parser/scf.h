#ifndef _SCF_H_
#define _SCF_H_
#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////
// Note(pjako): scf is a simplified version of
//

typedef enum scf_Type {
    scf_type__invalid,
    scf_type_category,
    scf_type_num,
    scf_type_str,
    scf_type_true,
    scf_type_false,
} scf_Type;

typedef struct scf_Global {
    Str8     fileName;
    Str8     str;
    u64      line;
    u64      column;

    u64      needle;
    u64      nextOffset;

    Str8     category;
    Str8     key;
    scf_Type valueType;
    Str8     valueStr;
} scf_Global;

API bool scf_next(scf_Global* scfGlobal);

API scf_Global scf_parse(Str8 scfStr, Str8 fileName);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _SCF_H_