#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include "../glad/glad.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

/* struct characterizing quad */
typedef struct Quad {
    float x; // x-coordinate of bottom left corner of quad (in pixels)
    float y; // y-coordinate of bottom left corner of quad (in pixels)
    float width; // width of quad (in pixels);
    float height; // height of quad (in pixels);
} Quad_t;

/* struct for TrueType character */
typedef struct Character {
    unsigned int TextureID;
    vec2 Sz;
    vec2 Bearing;
    unsigned int Advance;
} Character_t;

unsigned int create_shader(const char *vertex_shader, const char *fragment_shader);
void render_quad(unsigned int shader, Quad_t *q, vec3 color, unsigned int VAO, unsigned int VBO, unsigned int EBO);
void initialize_characters(unsigned int shader);
void render_text(unsigned int shader, char *text, float x, float y, float scale, vec3 color, unsigned int VAO, unsigned int VBO);
int pixel_in_quad(Quad_t *quad, float pix_x, float pix_y, float scr_width_init, float scr_height_init, float scr_width, float scr_height);
float text_width(char *text, float scale);
void initialize_quad(Quad_t *q, float x, float y, float width, float height);

#endif
