#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "cpu.h"
#include "isa.h"
#include "files.h"



typedef enum {
    ST_FREE,
    ST_INSTR_FETCH,
    ST_ERROR,
    ST_FINISH
} PARSE_STAGE;


typedef enum {
    TK_INSTRUCTION,
    TK_REGISTER,
    TK_IMMEDIATE,
    TK_NEWLINE,
    TK_COMMA,
    TK_END,
    TK_BULLSHIT,
} TOKENTYPE;


struct instr_metadata {
    char name[10];
    TOKENTYPE template[4];
    int template_len;
};

static const struct instr_metadata instr_md_table[INSTR_END] = {
    [ADDI] = { "addi", { TK_REGISTER, TK_COMMA, TK_IMMEDIATE, TK_NEWLINE }, 4 },
    [ADD] = { "add", { TK_REGISTER, TK_COMMA, TK_REGISTER, TK_NEWLINE }, 4 }
};


typedef struct {
    PARSE_STAGE stage;

    INSTR target_instr;
    int instr_tpl_step;

    int current_line;
    int current_column;

    unsigned short instr_buffer;
    bool should_write;
} ParserContext;


typedef struct {
    TOKENTYPE type;
    union {
        int reg_number;
        int imm;
        INSTR instr_type;
    };

    int length;
} Token;



Token get_token(char **cursor) {
    char *work_mem = *cursor;
    Token token;
    token.type = TK_BULLSHIT;

    for (;;work_mem++) {
        if (*work_mem != ' ') {
            break;
        }
    }

    token.length = 1;
    for (;;token.length++) {
        char symbol = work_mem[token.length];
        if (isspace(symbol) || symbol == '\0' || symbol == ',') {
            break;
        }
    }

    char symbol = *work_mem;

    if (symbol == '\0') {
        token.type = TK_END;
        return token;
    }


    char lookahead = *(work_mem+1);
    if (symbol == ',') {
        token.type = TK_COMMA;
    }
    else if (symbol == '\r' && lookahead == '\n') {
        token.type = TK_NEWLINE;
        token.length++;
    }
    // regs
    else if ((token.length == 2) && (symbol == 'x' || symbol == 'X') && (lookahead >= '0' && lookahead <= '8')) {
        token.type = TK_REGISTER;
        token.reg_number = lookahead - '0';
    }
    // immediate values 
    else if (symbol >= '0' && symbol <= '9') {
        char *bad_addr;
        int tmp = (int)strtol(work_mem, &bad_addr, 0);
        if (bad_addr != work_mem && isspace(*bad_addr)) {
            token.type = TK_IMMEDIATE;
            token.imm = tmp;
        }
    }
    // probably an instruction then
    else {
        for (int i = 0; i < INSTR_END; i++) {
            int len = strlen(instr_md_table[i].name);
            if (len == token.length && strncmp(work_mem, instr_md_table[i].name, token.length) == 0) {
                token.type = TK_INSTRUCTION;
                token.instr_type = i;
                break;
            }
        }
    }

    *cursor = work_mem + token.length;
    return token;
}


void stage_free(ParserContext *ctx, Token *tk);
void stage_instr_fetch(ParserContext *ctx, Token *tk);
void stage_finish(ParserContext *ctx, Token *tk);

void assemble(const wchar_t *filename) {
    FileMap source;
    get_file(filename, &source, 'r', true);

    char *cursor = source.map;
    ParserContext ctx = {0};
    ctx.stage = ST_FREE;
    ctx.current_line = 1;
    ctx.current_column = 1;

    while (ctx.stage != ST_FINISH && ctx.stage != ST_ERROR) {
        Token tk = get_token(&cursor);
        ctx.current_column += tk.length;

        if (tk.type == TK_NEWLINE) {
            ctx.current_column = 1;
            ctx.current_line++;
        }

        switch (ctx.stage) {
            case ST_FREE:
            stage_free(&ctx, &tk);
            break;

            case ST_INSTR_FETCH:
            stage_instr_fetch(&ctx, &tk);
            break;

            default:
            break;
            // case ST_FINISH:
            // stage_finish(&ctx, &tk);
            // break;
        }

    }
    if (ctx.stage == ST_FINISH) {
        printf("File assembled successfully. Lines read: %d:%d\n", ctx.current_line, ctx.current_column);
    }
    else {
        printf("File assembly failed due to the error above.\n");
    }

    close_file(&source);
}


void stage_free(ParserContext *ctx, Token *tk) {
    if (tk->type == TK_INSTRUCTION) {
        ctx->stage = ST_INSTR_FETCH;
        ctx->target_instr = tk->instr_type;
        ctx->instr_tpl_step = 0;
    }
    else if (tk->type == TK_END) {
        ctx->stage = ST_FINISH;
    }
    else if (tk->type == TK_BULLSHIT) {
        ctx->stage = ST_ERROR;
        printf("Unidentified symbol at line %d:%d\n", ctx->current_line, ctx->current_column);
    }
}


void stage_instr_fetch(ParserContext *ctx, Token *tk) {
    const struct instr_metadata *md = &instr_md_table[ctx->target_instr];

    bool last_instr = tk->type == TK_END? true : false;
    tk->type = last_instr? TK_NEWLINE : tk->type;

    if (tk->type == md->template[ctx->instr_tpl_step]) {
        ctx->instr_tpl_step++;
        printf("%d ", tk->type);
        if (ctx->instr_tpl_step == md->template_len) {
            ctx->stage = last_instr? ST_FINISH : ST_FREE;
        }
        return;
    }
    printf("Instruction syntax error at line %d:%d\n", ctx->current_line, ctx->current_column);
    ctx->stage = ST_ERROR;
}


void stage_finish(ParserContext *ctx, Token *tk) {

}