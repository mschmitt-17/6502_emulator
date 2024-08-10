#include <stdlib.h>
#include <stdio.h>

#include "graphics.h"
#include "../lib/lib.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define DEFAULT_CHARACTER_SIZE      24

/* buffer for character structs */
Character_t Characters[128];

/* create_shader
 *      DESCRIPTION: compiles vertex and fragment shaders and links into shader program
 *      INPUTS: vertex_shader -- file path to vertex shader
 *              fragment_shader -- file path to fragment shader
 *      OUTPUTS: shader program ID
 *      SIDE EFFECTS: none
 */
unsigned int create_shader(const char *vertex_shader, const char *fragment_shader) {
    int success;
    char info_log[512];
    
    const char *vertex_str = (const char *)read_file(vertex_shader);
    const char *fragment_str = (const char *)read_file(fragment_shader);

    // compile vertex shader
    unsigned int vertexID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexID, 1, &vertex_str, NULL);
    glCompileShader(vertexID);
    glGetShaderiv(vertexID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexID, 512, NULL, info_log);
        fprintf(stderr, "Vertex shader compilation failed\n");
        fprintf(stderr, "%s\n", info_log);
        exit(ERR_GRAPHICS);
    }

    // compile fragment shader
    unsigned int fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentID, 1, &fragment_str, NULL);
    glCompileShader(fragmentID);
    glGetShaderiv(fragmentID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentID, 512, NULL, info_log);
        fprintf(stderr, "Fragment shader compilation failed\n");
        fprintf(stderr, "%s\n", info_log);
        exit(ERR_GRAPHICS);
    }

    // link into shader program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexID);
    glAttachShader(program, fragmentID);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("Shader program compilation failed\n");
        printf("%s\n", info_log);
    }

    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);

    return program;
}

/* initialize_quad
 *      DESCRIPTION: initialzes fields of passed quad
 *      INPUTS: q -- quad to initialize
 *              x -- value to initialize x field of quad to
 *              x -- value to initialize x field of quad to
 *              width -- value to initialize width field of quad to
 *              height -- value to initialize height field of quad to
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies all fields of passed quad
 */
void initialize_quad(Quad_t *q, float x, float y, float width, float height) {
    q->x = x;
    q->y = y;
    q->width = width;
    q->height = height;
}

/* render_quad
 *      DESCRIPTION: renders passed quad to window
 *      INPUTS: shader -- shader program to use when rendering quad
 *              q -- pointer to quad to render
 *              color -- vector of color quad is to be rendered with
 *              VAO -- vertex array object to use when rendering quad
 *              VBO -- vertex buffer object to use when rendering quad
 *              EBO -- element buffer object to use when rendering quad
 *      OUTPUTS: none
 *      SIDE EFFECTS: renders quad to window
 */
void render_quad(unsigned int shader, Quad_t *q, vec3 color, unsigned int VAO, unsigned int VBO, unsigned int EBO) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "color"), color[0], color[1], color[2]);
    glBindVertexArray(VAO);
    
    float vertices[] = {
        q->x, q->y + q->height, // top left corner
        q->x, q->y, // bottom left corner
        q->x + q->width, q->y + q->height, // top right corner
        q->x + q->width, q->y // bottom right corner
    };

    unsigned int indices[]  = {
        0, 1, 2,
        1, 2, 3
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/* initialize_characters
 *      DESCRIPTION: populates global Characters buffer with first 128 characters of font
 *      INPUTS: shader -- shader to use for character textures
 *      OUTPUTS: none
 *      SIDE EFFECTS: populates Characters buffer with TrueType data
 */
void initialize_characters(unsigned int shader) {
    // set up FreeType, character stuff
    glUseProgram(shader);

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Failed to load FreeType library\n");
        exit(ERR_GRAPHICS);
    }

    FT_Face face;
    if (FT_New_Face(ft, "graphics/fonts/PressStart2P-Regular.ttf", 0, &face)) {
        fprintf(stderr, "Failed to load FreeType font\n");
        exit(ERR_GRAPHICS);
    }

    FT_Set_Pixel_Sizes(face, 0, DEFAULT_CHARACTER_SIZE);

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            printf("Error: failed to load glyph\n");
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Characters[c].TextureID = texture;
        Characters[c].Sz[0] = face->glyph->bitmap.width;
        Characters[c].Sz[1] = face->glyph->bitmap.rows;
        Characters[c].Bearing[0] = face->glyph->bitmap_left;
        Characters[c].Bearing[1] = face->glyph->bitmap_top;
        Characters[c].Advance = face->glyph->advance.x;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

/* render_text
 *      DESCRIPTION: renders passed text to window
 *      INPUTS: shader -- shader to use for rendered text
 *              text -- string to render to window
 *              x -- on-screen x-coordinate of bottom left corner of text to render
 *              y -- on-screen y-coordinate of bottom left corner of text to render
 *              scale -- factor of default font size to render text with (24 (default) * 0.5 (scale) = 12)
 *              color -- vector of color to use when rendering text
 *              VAO -- vertex attribute object to use when rendering text
 *              VBO -- vertex buffer object to use when rendering text
 *      OUTPUTS: none
 *      SIDE EFFECTS: renders text to window
 */
void render_text(unsigned int shader, char *text, float x, float y, float scale, vec3 color, unsigned int VAO, unsigned int VBO) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color[0], color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (int i = 0; i < strlen(text); i++) {
        float xpos = x + Characters[text[i]].Bearing[0] * scale;
        float ypos = y - (Characters[text[i]].Sz[1] - Characters[text[i]].Bearing[1]) * scale;
        float w = Characters[text[i]].Sz[0] * scale;
        float h = Characters[text[i]].Sz[1] * scale;

        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}
        };

        glBindTexture(GL_TEXTURE_2D, Characters[text[i]].TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (Characters[text[i]].Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* text_width
 *      DESCRIPTION: calculates width of passed string
 *      INPUTS: text -- string to calculate width of
 *              scale -- scale of text to calculate width of
 *      OUTPUTS: width (in pixels) of passed string
 *      SIDE EFFECTS: none
 */
float text_width(char *text, float scale) {
    float width = 0.0f;
    for (int i = 0; i < strlen(text); i++) {
        width += (Characters[text[i]].Advance >> 6) * scale;
    }
    return width;
}

/* pixel_in_quad
 *      DESCRIPTION: determines if passed pixel is in passed quad (used to determine if clicks are within certain quads)
 *      INPUTS: q -- pointer to quad which we wish to determine contained the passed pixel or not
 *              pix_x -- x-coordinate of pixel
 *              pix_y -- y-coordinate of pixel
 *              scr_width_init -- initial width of screen (before any resizing)
 *              scr_height_init -- initial height of screen (before any resizing)
 *              scr_width -- current width of screen (after resizing)
 *              scr_height -- current height of screen (after resizing)
 *      OUTPUTS: 1 if pixel is in quad, 0 otherwise
 *      SIDE EFFECTS: none
 */
int pixel_in_quad(Quad_t *q, float pix_x, float pix_y, float scr_width_init, float scr_height_init, float scr_width, float scr_height) {
    if (pix_x <= ((q->x + q->width) * (scr_width/scr_width_init)) &&
        pix_x >= ((q->x) * (scr_width/scr_width_init)) &&
        pix_y <= ((q->y + q->height) * (scr_height/scr_height_init)) &&
        pix_y >= ((q->y) * (scr_height/scr_height_init))) {
        return 1;
    }
    return 0;
}
