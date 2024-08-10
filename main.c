#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "test_code/tests.h"
#include "lib/lib.h"
#include "assembler/generator.h"
#include "assembler/scanner.h"
#include "graphics/graphics.h"

#define TABLE_INIT_SIZE         256
#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           600
#define NUM_MEM_LOCATIONS       16

// #define RUN_TESTS

// Flags
volatile uint8_t mouse_down = 0; // flag for if mouse button has been pressed and not released
volatile uint8_t click = 0; // flag for if mouse button has been pressed and released (full click)
volatile uint8_t run = 0; // flag for if run button has been clicked
volatile uint8_t user_entry = 0; // flag for if user entry field has been clicked
volatile uint8_t backspace_pressed = 0; // flag for if backspace has been pressed and not released
volatile uint8_t enter_pressed = 0; // flag for if enter has been pressed and not released

// Application variables
float curr_width = SCREEN_WIDTH; // current width of screen in pixels
float curr_height = SCREEN_HEIGHT; // current height of screen in pixels
char user_entry_buf[7] = "\0\0\0\0\0\0\0"; // buffer for user entry in search field
char user_entry_index = 0; // index in user_entry_buf
uint16_t starting_memory_location = 0x0000; // memory location to begin displayed memory locations from
uint8_t hex_chars[17] = {GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5,
                        GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_A, GLFW_KEY_B,
                        GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F, GLFW_KEY_X}; // buffer containing GLFW codes for hex keys    
uint8_t hex_pressed[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"; // buffer indicating if hex character corresponding to character in GLFW code buffer has been pressed and not released

// Helper functions
/* hex_to_char
 *      DESCRIPTION: converted passed hex number to character
 *      INPUTS: hex_number -- hex number (0-F) to convert to character
 *      OUTPUTS: hex number converted to character or -1 if character is not in 0-F range
 *      SIDE EFFECTS: none
 */
static uint8_t hex_to_char(uint8_t hex_number) {
    if (hex_number > 0x0F) {
        return -1;
    } else {
        if (hex_number < 0x0A) {
            return hex_number + 0x30;
        } else {
            return hex_number + 0x37;
        }
    }
}

/* fill_string
 *      DESCRIPTION: adds converted hex characters to end of passed string (used for register/memory values)
 *      INPUTS: str -- string to add hex characters to
 *              reg_val -- register value we wish to add to end of passed string
 *              num_chars -- number of characters in register we wish to add to end of passed string (2 for 8-bit register, 4 for 16-bit register)
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds num_chars converted hex characters to end of str
 */
static void fill_string(char *str, uint16_t reg_val, uint8_t num_chars) {
    for (int i = 0; i < num_chars; i++) {
        str[i] = hex_to_char((reg_val & (0xF << (4 * (num_chars - i - 1)))) >> (4 * (num_chars - i - 1)));
    }
}

/* check_hex_characters
 *      DESCRIPTION: checks if any hex characters have been pressed by user, adjusts user entry buffer accordingly
 *      INPUTS: window -- pointer to window object for emulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds pressed hex characters to user_entry_buf if user_entry_buf isn't full
 */
static void check_hex_characters(GLFWwindow *window) {
    for (int i = 0; i < 17; i++) {
        if (glfwGetKey(window, hex_chars[i]) == GLFW_PRESS) {
            hex_pressed[i] = 1;
        } else if ((glfwGetKey(window, hex_chars[i]) == GLFW_RELEASE) && hex_pressed[i]) {
            hex_pressed[i] = 0;
            if (user_entry_index < 6) {
                if (hex_chars[i] == GLFW_KEY_X) {
                    user_entry_buf[user_entry_index++] = hex_chars[i] + 0x20;
                } else {
                    user_entry_buf[user_entry_index++] = hex_chars[i];
                }
            }
        }
    }
}

/* check_user_input
 *      DESCRIPTION: checks to see if user_entry_buf is valid absolute address, if so adjusts starting_memory_location
                     to nearest 16 memory location increment
        INPUTS: none
        OUTPUTS: none
        SIDE EFFECTS: adjusts starting_memory_location if user_entry_buf contains valid address, clears user_entry_buf
 */
static void check_user_input() {
    if (user_entry_index == 6 &&
        user_entry_buf[0] == '0' &&
        user_entry_buf[1] == 'x' &&
        is_hex_number(user_entry_buf[2]) &&
        is_hex_number(user_entry_buf[3]) &&
        is_hex_number(user_entry_buf[4]) &&
        is_hex_number(user_entry_buf[5])) {
        starting_memory_location = 0x0000;
        starting_memory_location |= (char_to_hex(user_entry_buf[2]) << 12);
        starting_memory_location |= (char_to_hex(user_entry_buf[3]) << 8);
        starting_memory_location |= (char_to_hex(user_entry_buf[4]) << 4);
        starting_memory_location |= char_to_hex(user_entry_buf[5]);
        starting_memory_location = starting_memory_location/16 * 16;
    }
    user_entry_index = 0;
    user_entry = 0;
    memset(user_entry_buf, '\0', 7);
}

/* load_program
 *      DESCRIPTION: loads user program specified via command line at locations specified by assembly
 *      INPUTS: sf -- pointer to 6502 struct running program
 *              file_path -- string for file path of assembly to run
 *      OUTPUTS: none
 *      SIDE EFFECTS: resets memory and loads with bytecode, resets all registers to initial values
 */
static void load_program(sf_t *sf, char *file_path) {
    memset(sf->memory, '\0', MEMORY_SIZE);

    uint8_t *sf_asm = read_file(file_path);
    Table_t *label_table = new_table(TABLE_INIT_SIZE);

    Clip_t *c = assembly_to_clip(sf, sf_asm, &label_table);
    Program_t *p = clip_to_program(sf_asm, c, &label_table);

    
    for (int i = 0; i < p->index; i++) {
        load_bytecode(sf, p->start + i, p->start[i].load_address, p->start[i].index);
    }

    initialize_regs(sf, p->start[0].load_address);

    free_clip(c);
    free_program(p);
    free_table(label_table);
    free(sf_asm);
}

/* processInput
 *      DESCRIPTION: processes user key presses
 *      INPUTS: window -- pointer to window object for emulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets flags, calls functions depending on which key was pressed
 */
static void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    } else if ((glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) && user_entry) {
        enter_pressed = 1;
    } else if ((glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) && enter_pressed && user_entry) {
        enter_pressed = 0;
        check_user_input();
    } else if ((glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) && user_entry) {
        backspace_pressed = 1;
    } else if ((glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_RELEASE) && backspace_pressed && user_entry) {
        backspace_pressed = 0;
        user_entry_buf[--user_entry_index] = '\0';
    } else if (user_entry) {
        check_hex_characters(window);
    }
}

// Callback functions
/* framebuffer_size_callback
 *      DESCRIPTION: callback for when user resizes window; resizes window and adjusts width and height variables
 *      INPUTS: window -- pointer to window object for emulator
 *              width -- new width of window
 *              height -- new height of window
 *      OUTPUTS: none
 *      SIDE EFFECTS: resizes window, modifies width and height variables
 */
static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    curr_width = width;
    curr_height = height;
}

/* mouse_button_callback
 *      DESCRIPTION: callback for when mouse button is pressed/released
 *      INPUTS: window -- pointer to window object for emulator
 *              button -- button that was pressed by user
 *              action -- press, release, repeat
 *              mods -- modifier buttons which were also held down
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets mouse_down, click flags depending on if button has been pressed or released
 */
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        mouse_down = 1;
    } else if (action == GLFW_RELEASE && mouse_down) {
        mouse_down = 0;
        click = 1;
    }
}

int main(int argc, char* argv[]) {
    sf_t *sf = (sf_t *)malloc(sizeof(sf_t));
#ifdef RUN_TESTS
    run_opcode_tests(sf);
    table_test();
#else
    if (argc == 1) {
        fprintf(stderr, "Error: must enter an assembly file to run\n");
        exit(ERR_NO_FILE);
    }

    load_program(sf, argv[1]);

    // strings for register values, memory values
    char accumulator_str[20] = "Accumulator: 0x00";
    char x_index_str[14] = "X Index: 0x00";
    char y_index_str[14] = "Y Index: 0x00";
    char status_str[13] = "Status: 0x00";
    char esp_str[22] = "Stack Pointer: 0x0000";
    char pc_str[11] = "PC: 0x0000";
    char memory_string[16] = "0x0000: 0x0000";

    // initialize and configure GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create window object
    GLFWwindow *window = glfwCreateWindow(800, 600, "6502 Emulator", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW Window\n");
        glfwTerminate();
        exit(ERR_GRAPHICS);
    }

    // make window context main context on current thread
    glfwMakeContextCurrent(window);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        exit(ERR_GRAPHICS);
    }

    // tell OpenGL size of rendering window
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // set callback for when window size is adjusted
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // enable desired OpenGL settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // compile and set up shaders
    unsigned int text_shader = create_shader("graphics/shaders/textShader.vs", "graphics/shaders/textShader.fs");
    unsigned int quad_shader = create_shader("graphics/shaders/quadShader.vs", "graphics/shaders/quadShader.fs");

    // set up projection matrix for shaders
    mat4 projection;
    glm_ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT, -0.1f, 0.1f, projection);

    // load textures for characters
    initialize_characters(text_shader);

    // set up VAO, VBO for text
    glUseProgram(text_shader);
    unsigned int VAO_text, VBO_text;
    glGenVertexArrays(1, &VAO_text);
    glGenBuffers(1, &VBO_text);
    glBindVertexArray(VAO_text);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_text);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // set up VAO, VBO, EBO for quads
    glUseProgram(quad_shader);
    unsigned int VAO_quad, EBO_quad, VBO_quad;
    glGenVertexArrays(1, &VAO_quad);
    glGenBuffers(1, &EBO_quad);
    glGenBuffers(1, &VBO_quad);
    glBindVertexArray(VAO_quad);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_quad);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3 * 2, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // initialize quads for region of screen displaying memory locations and their values
    float mem_region_top = 2 * (SCREEN_HEIGHT/3); // y-coordinate of top of region of screen displaying memory
    Quad_t memory_quads[NUM_MEM_LOCATIONS];
    for (int i = 0; i < NUM_MEM_LOCATIONS; i++) {
        initialize_quad(memory_quads + i, 0.0f, mem_region_top - ((i + 1) * (mem_region_top/NUM_MEM_LOCATIONS)), SCREEN_WIDTH, mem_region_top/NUM_MEM_LOCATIONS);
    }

    // initialize quads for buttons
    float button_region_top = SCREEN_HEIGHT - 4; // y-coordinate of top of region of screen with buttons
    Quad_t continue_quad;
    initialize_quad(&continue_quad, SCREEN_WIDTH / 2, button_region_top - ((SCREEN_HEIGHT / 15) + 4.0f), SCREEN_WIDTH / 5, SCREEN_HEIGHT / 15);
    Quad_t next_quad;
    initialize_quad(&next_quad, SCREEN_WIDTH / 2, button_region_top - (2 * ((SCREEN_HEIGHT / 15) + 4.0f)), SCREEN_WIDTH / 5, SCREEN_HEIGHT / 15);
    Quad_t search_quad;
    initialize_quad(&search_quad, SCREEN_WIDTH / 2 + 100.0f, button_region_top - (3 * ((SCREEN_HEIGHT / 15) + 4.0f)), SCREEN_WIDTH / 5, SCREEN_HEIGHT / 15);
    Quad_t enter_quad;
    initialize_quad(&enter_quad, search_quad.x + search_quad.width + 4.0f, search_quad.y, SCREEN_WIDTH / 15, SCREEN_HEIGHT / 15);
    Quad_t reset_quad;
    initialize_quad(&reset_quad, continue_quad.x + continue_quad.width + 4.0f, continue_quad.y, SCREEN_WIDTH / 5, SCREEN_HEIGHT / 15);

    while(!glfwWindowShouldClose(window)) {
        // process keyboard inputs
        processInput(window);

        // process button clicks
        if (click) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            user_entry = 0;
            if (pixel_in_quad(&continue_quad, xpos, curr_height - ypos, SCREEN_WIDTH, SCREEN_HEIGHT, curr_width, curr_height)) {
                run = 1;
            } else if (pixel_in_quad(&next_quad, xpos, curr_height - ypos, SCREEN_WIDTH, SCREEN_HEIGHT, curr_width, curr_height)) {
                process_line(sf);
            } else if (pixel_in_quad(&search_quad, xpos, curr_height - ypos, SCREEN_WIDTH, SCREEN_HEIGHT, curr_width, curr_height)) {
                user_entry = 1;
            } else if (pixel_in_quad(&enter_quad, xpos, curr_height - ypos, SCREEN_WIDTH, SCREEN_HEIGHT, curr_width, curr_height)) {
                check_user_input();
            } else if (pixel_in_quad(&reset_quad, xpos, curr_height - ypos, SCREEN_WIDTH, SCREEN_HEIGHT, curr_width, curr_height)) {
                load_program(sf, argv[1]);
                run = 0;
            }
            click = 0;
        }

        if (run) {
            process_line(sf);
        } 

        // rendering commands
        glClearColor(0.66f, 0.66f, 0.66f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render register information
        fill_string(accumulator_str + 17, sf->accumulator, 2);
        fill_string(x_index_str + 11, sf->x_index, 2);
        fill_string(y_index_str + 11, sf->y_index, 2);
        fill_string(status_str + 10, sf->status, 2);
        fill_string(esp_str + 17, sf->esp, 4);
        fill_string(pc_str + 6, sf->pc, 4);

        glBindVertexArray(VAO_text);
        glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
        render_text(text_shader, "Registers:", 2.0f, (float)SCREEN_HEIGHT - 26.0f, 1.0f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, accumulator_str, 2.0f, (float)SCREEN_HEIGHT - 44.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, x_index_str, 2.0f, (float)SCREEN_HEIGHT - 62.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, y_index_str, 2.0f, (float)SCREEN_HEIGHT - 80.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, status_str, 2.0f, (float)SCREEN_HEIGHT - 98.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, esp_str, 2.0f, (float)SCREEN_HEIGHT - 116.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        render_text(text_shader, pc_str, 2.0f, (float)SCREEN_HEIGHT - 134.0f, 0.75f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);

        // render buttons/search bar
        glBindVertexArray(VAO_quad);
        glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
        render_quad(quad_shader, &continue_quad, (vec3){0.33f, 0.33f, 0.33f}, VAO_quad, VBO_quad, EBO_quad);
        render_quad(quad_shader, &next_quad, (vec3){0.33f, 0.33f, 0.33f}, VAO_quad, VBO_quad, EBO_quad);
        if (user_entry) {
            render_quad(quad_shader, &search_quad, (vec3){1.0f, 1.0f, 1.0f}, VAO_quad, VBO_quad, EBO_quad);
        } else {
            render_quad(quad_shader, &search_quad, (vec3){0.33f, 0.33f, 0.33f}, VAO_quad, VBO_quad, EBO_quad);
        }
        render_quad(quad_shader, &enter_quad, (vec3){0.33f, 0.33f, 0.33f}, VAO_quad, VBO_quad, EBO_quad);
        render_quad(quad_shader, &reset_quad, (vec3){0.33f, 0.33f, 0.33f}, VAO_quad, VBO_quad, EBO_quad);
        glBindVertexArray(VAO_text);
        glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
        render_text(text_shader, "Run", continue_quad.x + ((continue_quad.width - text_width("Run", 0.5f))/2), continue_quad.y + ((continue_quad.height - 12)/2), 0.5f, (vec3){0.66f, 0.66f, 0.66f}, VAO_text, VBO_text);
        render_text(text_shader, "Next", next_quad.x + ((next_quad.width - text_width("Next", 0.5f))/2), next_quad.y + ((next_quad.height - 12)/2), 0.5f, (vec3){0.66f, 0.66f, 0.66f}, VAO_text, VBO_text);
        render_text(text_shader, "Search:", SCREEN_WIDTH/2, search_quad.y + ((search_quad.height - 12)/2), 0.5f, (vec3){0.33f, 0.33f, 0.33f}, VAO_text, VBO_text);
        render_text(text_shader, user_entry_buf, search_quad.x, search_quad.y + ((search_quad.height - 24)/2), 1.0f, (vec3){0.66f, 0.66f, 0.66f}, VAO_text, VBO_text);
        render_text(text_shader, "->", enter_quad.x + ((enter_quad.width - text_width("->", 0.5f))/2), enter_quad.y + ((enter_quad.height - 12)/2), 0.5f, (vec3){0.66f, 0.66f, 0.66f}, VAO_text, VBO_text);
        render_text(text_shader, "Reset", reset_quad.x + ((reset_quad.width - text_width("Reset", 0.5f))/2), reset_quad.y + ((reset_quad.height - 12)/2), 0.5f, (vec3){0.66f, 0.66f, 0.66f}, VAO_text, VBO_text);

        // render memory locations
        glBindVertexArray(VAO_text);
        glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
        render_text(text_shader, "Memory:", 2.0f, memory_quads[0].y + memory_quads[0].height, 1.0f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        for (int i = 0; i < NUM_MEM_LOCATIONS; i++) {
            fill_string(memory_string + 2, starting_memory_location + i, 4);
            fill_string(memory_string + 10, sf->memory[starting_memory_location + i], 4);
            // render background
            glBindVertexArray(VAO_quad);
            glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
            render_quad(quad_shader, memory_quads + i, (vec3){0.66f/(1 + i%2), 0.66f/(1 + i%2), 0.66f/(1 + i%2)}, VAO_quad, VBO_quad, EBO_quad);
            // render text
            glBindVertexArray(VAO_text);
            glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, (float *)projection);
            render_text(text_shader, memory_string, 2.0f, memory_quads[i].y, 0.66f, (vec3){1.0f, 0.0f, 0.0f}, VAO_text, VBO_text);
        }

        // check/call window events and swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

#endif
    
    free(sf);
    return 0;
}
