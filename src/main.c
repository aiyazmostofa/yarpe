#include <graphx.h>
#include <fileioc.h>
#include <stdio.h>
#include <ti/real.h>
#include <ti/getcsc.h>
#include "THIN_SS.h"

#define BG 0
#define FGP 1
#define FGS 2
#define TXT 3

#define STACK_SIZE 100
#define QUEUE_SIZE 50
#define INPUT_SIZE 20

// RPN
char input[INPUT_SIZE + 1];
real_t stack[STACK_SIZE];
real_t queue[QUEUE_SIZE];
int inputIndex;
int stackIndex;
int queueIndex;
bool containsDecimal;
bool negativeInput;

char backup_input[INPUT_SIZE + 1];
real_t backup_stack[STACK_SIZE];
real_t backup_queue[QUEUE_SIZE];
int backup_inputIndex;
int backup_stackIndex;
int backup_queueIndex;
bool backup_containsDecimal;
bool backup_negativeInput;

bool undo;

bool push();
bool append(char c);
bool back();
bool clear();
bool negate();
bool delete();
bool swap();
bool add();
bool poll();
void writeQueue();
void readQueue();
void backup();
bool restore();

// Math
bool prepare(int req);
bool calc_add();
bool calc_sub();
bool calc_mul();
bool calc_div();
bool calc_pow();
bool calc_square();
bool calc_sqrt();
bool calc_log();
bool calc_ln();
bool calc_exp();
bool calc_ten_pow();
bool calc_inv();
bool calc_sci();
bool calc_pi();
bool calc_euler();
bool calc_sin();
bool calc_cos();
bool calc_tan();
bool calc_asin();
bool calc_acos();
bool calc_atan();

// Game cycle
bool update;

void begin();
void end();
bool step();
void draw();
void set_theme();

// Settings
bool second;
bool alpha;
bool scientific;
bool radians;

// Themes
char themes [][4][3] = {
    // Layout: Background, Primary Foreground, Secondary Foreground, Text Color

    // Default
    {{0, 0, 0}, {255, 255, 255}, {155, 211, 221}, {255, 255, 255}},
    // Monkeytype Classic (Serika Dark)
    {{50, 52, 55}, {226, 183, 20}, {100, 102, 105}, {226, 183, 20}},
    // Dracula
    {{40, 42, 54}, {98, 114, 164}, {68, 71, 90}, {189, 147, 249}},
    // Gruvbox
    {{40, 40, 40}, {152, 151, 26}, {251, 73, 52}, {235, 219, 178}},
};
bool theme_set;
int theme_count = sizeof(themes) / (12 * sizeof(char));
int current_theme;
int saved_theme;
uint8_t theme_appvar_handle;

int main(void) {
    begin();
    gfx_Begin();
    gfx_ZeroScreen();
    gfx_SetDrawBuffer();

    set_theme();

    gfx_SetFontData(font);
    gfx_SetMonospaceFont(8);
    gfx_SetTextScale(2, 2);

    while (step()) {
        if (update) gfx_BlitScreen();
        draw();
        gfx_SwapDraw();
    }
    gfx_End();
    end();
    return 0;
}

void begin() {
    stackIndex = 0;
    queueIndex = 0;
    update = true;
    scientific = true;
    second = false;
    alpha = false;
    undo = false;
    radians = true;

    theme_set = false;
    theme_appvar_handle = ti_Open("RPNTHEME", "r");
    if (theme_appvar_handle == 0) {
        current_theme = 0;
    } else {
        size_t elements_written = ti_Read(&current_theme, sizeof(int), 1, theme_appvar_handle);
    }
    int close_error = ti_Close(theme_appvar_handle);
    readQueue();
    clear();
}

bool prepare(int req) {
    if (inputIndex > 0) {
        if (stackIndex + 1 < req) return false;
        backup();
        push();
        return true;
    } else if (stackIndex < req) return false;
    backup();
    return true;
}

bool step() {
    uint8_t key = os_GetCSC();
    if (key == sk_2nd || key == sk_LParen) {
        second = !second;
        update = true;
        return true;
    }
    
    if (key == sk_Alpha) {
        alpha = !alpha;
        theme_set = false;
        update = true;
        return true;
    }

    if (alpha) {
        if (key == sk_Window) {
            theme_set = true;
            update = true;
            return true;
        }
    }

    if (theme_set) {
        if (key == sk_Up) {
            current_theme = (current_theme + 1) % theme_count;
            set_theme();
            update = true;
            return true;
        }
        if (key == sk_Enter) {
            theme_set = false;
            uint8_t handle = ti_Open("RPNTHEME", "w");
            size_t elements_written = ti_Write(&current_theme, sizeof(int), 1, handle);
            int close_error = ti_Close(handle);

            alpha = false;
            update = true;
            return true;
        }
    }

    if (second) {
        if (key == sk_Mode || key == sk_RParen) return false;
        if (key == sk_Square && prepare(1) && calc_sqrt()) update = true;
        if (key == sk_Ln && prepare(1) && calc_exp()) update = true;
        if (key == sk_Log && prepare(1) && calc_ten_pow()) update = true;
        if (key == sk_Enter && restore()) update = true;
        if (key == sk_Power && calc_pi()) update = true;
        if (key == sk_Div && calc_euler()) update = true;

        if (key == sk_Sin && prepare(1) && calc_asin()) update = true;
        if (key == sk_Cos && prepare(1) && calc_acos()) update = true;
        if (key == sk_Tan && prepare(1) && calc_atan()) update = true;

        if (key != 0) {
            second = false;
            update = true;
        }
        return true;
    }

    if (key == sk_Mode) {
        scientific = !scientific;
        update = true;
    }

    if (key == sk_Enter && push()) update = true;
    if (key == sk_Clear && clear()) update = true;
    if (key == sk_Prgm && poll()) update = true;
    if ((key == sk_RParen || key == sk_Del) && back()) update = true;
    if (key == sk_Chs && negate()) update = true;
    if (key == sk_0 && append('0')) update = true;
    if (key == sk_1 && append('1')) update = true;
    if (key == sk_2 && append('2')) update = true;
    if (key == sk_3 && append('3')) update = true;
    if (key == sk_4 && append('4')) update = true;
    if (key == sk_5 && append('5')) update = true;
    if (key == sk_6 && append('6')) update = true;
    if (key == sk_7 && append('7')) update = true;
    if (key == sk_8 && append('8')) update = true;
    if (key == sk_9 && append('9')) update = true;
    if (key == sk_DecPnt && append('.')) update = true;

    if (key == sk_Apps) {
        radians = !radians;
        update = true;
    }

    if (key == sk_Vars && prepare(1) && add()) update = true;
    if (key == sk_Down && prepare(2) && swap()) update = true;

    if (key == sk_Add && prepare(2) && calc_add()) update = true;
    if (key == sk_Sub && prepare(2) && calc_sub()) update = true;
    if (key == sk_Mul && prepare(2) && calc_mul()) update = true;
    if (key == sk_Div && prepare(2) && calc_div()) update = true;
    if (key == sk_Power && prepare(2) && calc_pow()) update = true;

    if (key == sk_Square && prepare(1) && calc_square()) update = true;
    if (key == sk_Ln && prepare(1) && calc_ln()) update = true;
    if (key == sk_Log && prepare(1) && calc_log()) update = true;
    if (key == sk_Recip && prepare(1) && calc_inv()) update = true;
    if (key == sk_Comma && prepare(2) && calc_sci()) update = true;

    if (key == sk_Sin && prepare(1) && calc_sin()) update = true;
    if (key == sk_Cos && prepare(1) && calc_cos()) update = true;
    if (key == sk_Tan && prepare(1) && calc_tan()) update = true;

    return true;
}

bool push() {
    if (inputIndex == 0 || stackIndex == STACK_SIZE) return false;
    backup();
    stack[stackIndex++] = os_StrToReal(input, NULL);
    inputIndex = 0;
    if (negativeInput)
        stack[stackIndex - 1] = os_RealNeg(&stack[stackIndex - 1]);
    inputIndex = 1;
    clear();
    return true;
}

bool clear() {
    if (inputIndex == 0) return delete();
    inputIndex = 0;
    containsDecimal = false;
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = ' ';
    }
    negativeInput = false;
    input[INPUT_SIZE] = '\0';
    return true;
}

bool delete() {
    if (stackIndex == 0) return false;
    backup();
    stackIndex--;
    return true;
}

bool back() {
    if (inputIndex == 0) return delete();
    if (input[inputIndex - 1] == '.') containsDecimal = false;
    input[--inputIndex] = ' ';
    if (inputIndex == 0) negativeInput = false;
    return true;
}

bool append(char c) {
    if (inputIndex == INPUT_SIZE) return false;
    if (c == '.') {
        if (containsDecimal) return false;
        containsDecimal = true;
    }
    input[inputIndex++] = c;
    return true;
}

bool negate() {
    if (inputIndex > 0) {
        negativeInput = !negativeInput;
    } else if (stackIndex > 0) {
        backup();
        stack[stackIndex - 1] = os_RealNeg(&stack[stackIndex - 1]);
    } else return false;
    return true;
}

bool swap() {
    real_t temp = stack[stackIndex-1];
    stack[stackIndex-1] = stack[stackIndex-2];
    stack[stackIndex-2] = temp;
    temp = os_RealLog(&temp);
    return true;
}

bool add() {
    if (queueIndex == QUEUE_SIZE) return false;
    queue[queueIndex++] = stack[--stackIndex];
    writeQueue();
    return true;
}

bool poll() {
    if (queueIndex == 0 || STACK_SIZE - stackIndex < queueIndex) return false;
    backup();
    for (int i = 0; i < queueIndex; i++) {
        stack[stackIndex++] = queue[i];
    }
    queueIndex = 0;
    writeQueue();
    return true;
}

void writeQueue() {
    uint8_t handle = ti_Open("QUEUE", "w");
    size_t elements_written = ti_Write(&queue, sizeof(real_t[QUEUE_SIZE]), 1, handle);
    int close_error = ti_Close(handle);

    handle = ti_Open("QUEUEINDEX", "w");
    elements_written = ti_Write(&queueIndex, sizeof(int), 1, handle);
    close_error = ti_Close(handle);
}

void readQueue() {
    uint8_t handle = ti_Open("QUEUE", "r");
    if (handle != 0) {
        size_t elements_written = ti_Read(&queue, sizeof(real_t[QUEUE_SIZE]), 1, handle);
    }
    int close_error = ti_Close(handle);


    handle = ti_Open("QUEUEINDEX", "r");
    if (handle != 0) {
        size_t elements_written = ti_Read(&queueIndex, sizeof(int), 1, handle);
    }
    close_error = ti_Close(handle);
}

void backup() {
    undo = true;
    for (int i = 0; i <= INPUT_SIZE; i++) {
        backup_input[i] = input[i];
    }

    for (int i = 0; i < STACK_SIZE; i++) {
        backup_stack[i] = stack[i];
    }

    for (int i = 0; i < QUEUE_SIZE; i++) {
        backup_queue[i] = queue[i];
    }

    backup_inputIndex = inputIndex;
    backup_stackIndex = stackIndex;
    backup_queueIndex = queueIndex;
    backup_containsDecimal = containsDecimal;
    backup_negativeInput = negativeInput;
}

bool restore() {
    if (!undo) return false;
    undo = false;
    for (int i = 0; i <= INPUT_SIZE; i++) {
        input[i] = backup_input[i];
    }
    
    for (int i = 0; i < STACK_SIZE; i++) {
            stack[i] = backup_stack[i];
    }
    
    for (int i = 0; i < QUEUE_SIZE; i++) {
        queue[i] = backup_queue[i];
    }
    
    inputIndex = backup_inputIndex;
    stackIndex = backup_stackIndex;
    queueIndex = backup_queueIndex;
    containsDecimal = backup_containsDecimal;
    negativeInput = backup_negativeInput;
    return true;
}

bool calc_add() {
    stack[stackIndex - 2] = os_RealAdd(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_sub() {
    stack[stackIndex - 2] = os_RealSub(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_mul() {
    stack[stackIndex - 2] = os_RealMul(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_div() {
    stack[stackIndex - 2] = os_RealDiv(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_pow() {
    stack[stackIndex - 2] = os_RealPow(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_square() {
    stack[stackIndex - 1] = os_RealMul(&stack[stackIndex - 1], &stack[stackIndex - 1]);
    return true;
}

bool calc_sqrt() {
    stack[stackIndex - 1] = os_RealSqrt(&stack[stackIndex - 1]);
    return true;
}

bool calc_log() {
    real_t temp = os_FloatToReal(10.0f);
    temp = os_RealLog(&temp);
    stack[stackIndex - 1] = os_RealLog(&stack[stackIndex - 1]);
    stack[stackIndex - 1] = os_RealDiv(&stack[stackIndex - 1], &temp);
    return true;
}

bool calc_ln() {
    stack[stackIndex - 1] = os_RealLog(&stack[stackIndex - 1]);
    return true;
}

bool calc_exp() {
    stack[stackIndex - 1] = os_RealExp(&stack[stackIndex - 1]);
    return true;
}

bool calc_ten_pow() {
    real_t temp = os_FloatToReal(10.0f);
    stack[stackIndex - 1] = os_RealPow(&temp, &stack[stackIndex - 1]);
    return true;
}

bool calc_inv() {
    stack[stackIndex - 1] = os_RealInv(&stack[stackIndex - 1]);
    return true;
}

bool calc_sci() {
    real_t temp = os_FloatToReal(10.0f);
    stack[stackIndex - 1] = os_RealPow(&temp, &stack[stackIndex - 1]);
    stack[stackIndex - 2] = os_RealMul(&stack[stackIndex - 2], &stack[stackIndex - 1]);
    stackIndex--;
    return true;
}

bool calc_pi() {
    if (stackIndex == STACK_SIZE) return false;
    stack[stackIndex++] = os_FloatToReal(3.141592654f);
    return true;
}

bool calc_euler() {
    if (stackIndex == STACK_SIZE) return false;
    stack[stackIndex++] = os_FloatToReal(2.718281828f);
    return true;
}

bool calc_sin() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealSinRad(&temp);
    return true;
}

bool calc_cos() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealCosRad(&temp);
    return true;
}

bool calc_tan() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealTanRad(&temp);
    return true;
}

bool calc_asin() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealAsinRad(&temp);
    return true;
}


bool calc_acos() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealAcosRad(&temp);
    return true;
}

bool calc_atan() {
    real_t temp = stack[stackIndex - 1];
    if (!radians) temp = os_RealDegToRad(&temp);
    stack[stackIndex - 1] = os_RealAtanRad(&temp);
    return true;
}

void draw() {
    int FG;
    if (second) FG = FGS;
    else FG = FGP;

    // Background
    gfx_SetColor(BG);
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, GFX_LCD_HEIGHT);

    gfx_SetColor(FG);
    // Input prompt
    if (negativeInput) {
        gfx_FillRectangle(0, 220, GFX_LCD_WIDTH, 20);
        gfx_SetTextFGColor(BG);
        gfx_PrintStringXY(input, 0, 222);
        gfx_SetTextFGColor(TXT);
    } else {
        gfx_Line(0, 220, GFX_LCD_WIDTH, 220);
        gfx_SetTextFGColor(TXT);
        gfx_PrintStringXY(input, 0, 222);
    }

    // Radians/degree toggle display
    if (radians) gfx_PrintStringXY("R", 304, 0);
    else gfx_PrintStringXY("D", 304, 0);

    if (queueIndex >= 10) {
        char str[3];
        str[2] = '\0';
        sprintf(str, "%d", queueIndex);
        gfx_PrintStringXY(str, 288, 16);
    } else {
        char str[2];
        str[1] = '\0';
        sprintf(str, "%d", queueIndex);
        gfx_PrintStringXY(str, 304, 16);
    }

    // Stack
    int i = stackIndex - 1;
    for (int y = 202; y >= 0 && i >= 0; y -= 20) {
        char result[14];
        if (scientific) {
            os_RealToStr(result, &stack[i], 0, 2, 2);
        } else {
            os_RealToStr(result, &stack[i], 0, 1, -1);
        }
        gfx_PrintStringXY(result, 0, y);
        i--;
    }
    update = false;
}

void set_theme() {
    // Background
    gfx_palette[BG] = gfx_RGBTo1555(themes[current_theme][BG][0], themes[current_theme][BG][1], themes[current_theme][BG][2]);
    // Primary Foreground
    gfx_palette[FGP] = gfx_RGBTo1555(themes[current_theme][FGP][0], themes[current_theme][FGP][1], themes[current_theme][FGP][2]);
    // Secondary Foreground
    gfx_palette[FGS] = gfx_RGBTo1555(themes[current_theme][FGS][0], themes[current_theme][FGS][1], themes[current_theme][FGS][2]);
    // Text
    gfx_palette[TXT] = gfx_RGBTo1555(themes[current_theme][TXT][0], themes[current_theme][TXT][1], themes[current_theme][TXT][2]);
}

void end() {
}
