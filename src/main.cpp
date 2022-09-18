#include <epoxy/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtk/gtk.h>
#include <libserialport.h>

GLuint buffer;
GLuint program;
glm::mat4 model = glm::mat4(1.0);
struct sp_port *main_port;

enum MOVEMENT_TYPE {
  START_DRILL,
  STOP_DRILL,
  UP,
  DOWN
};

typedef struct command {
  int movement_type;
} command;

const GLfloat vertices[] = {
    1, -1, -1, 0, -1, 0, 1, -1, 1, 0, -1, 0, -1, -1, 1, 0, -1, 0, 1, -1, -1, 0,
    -1, 0, -1, -1, 1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 1, 1, 0, 1, 0, 1, 1, 1,
    0, 1, 0, 1, 1, -1, 0, 1, 0, -1, 1, 1, 0, 1, 0, 1, 1, -1, 0, 1, 0, -1, 1, -1,
    0, 1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1,
    -1, 1, -1, 0, 0, -1, 1, 1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, -1, 1, 0, 0, 1,
    1, -1, 1, 0, 0, 1, -1, 1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
    -1, 1, 1, 0, 0, 1, 1, 1, -1, 0, 0, -1, 1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0,
    -1, 1, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 1, -1, 0, 0, -1, 1, 1, 1,
    1, 0, 0, 1, -1, 1, 1, 0, 0, 1, -1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, -1, -1,
    1, 0, 0, 1, 1, -1, 1, 0, 0};

GLuint init_shader(int type) {

  GLuint shader = glCreateShader(type);

  if (type == GL_FRAGMENT_SHADER) {
    FILE *f = fopen("shaders/fragment.sl", "rb");
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    rewind(f);
    char *buf = (char *)malloc(s + 1);
    bzero(buf, s + 1);
    fread(buf, 1, s, f);
    glShaderSource(shader, 1, &buf, NULL);
    fclose(f);
  }

  if (type == GL_VERTEX_SHADER) {
    FILE *f = fopen("shaders/vertex.sl", "rb");
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    rewind(f);
    char *buf = (char *)malloc(s + 1);
    bzero(buf, s + 1);
    fread(buf, 1, s, f);
    glShaderSource(shader, 1, &buf, NULL);
    fclose(f);
  }

  glCompileShader(shader);

  int status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

  return shader;
}

void init_shaders(GLuint *program) {
  GLuint vertex, fragment;

  vertex = init_shader(GL_VERTEX_SHADER);
  fragment = init_shader(GL_FRAGMENT_SHADER);

  *program = glCreateProgram();
  glAttachShader(*program, vertex);
  glAttachShader(*program, fragment);

  glLinkProgram(*program);

  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void realize(GtkWidget *widget) {
  gtk_gl_area_make_current(GTK_GL_AREA(widget));

  glGenVertexArrays(1, &buffer);
  glBindVertexArray(buffer);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  init_shaders(&program);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);
}

void draw(GLuint program) {
  glUseProgram(program);

  glm::vec3 position = glm::vec3(0, 0, 5);
  glm::vec3 front = glm::vec3(0, 0, -1);
  glm::vec3 up = glm::vec3(0, 1, 0);

  model = rotate(model, .01f, glm::vec3(1, 1, 1));
  //model = translate(model, glm::vec3(.1, 0, 0));
  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

  glm::mat4 view = glm::lookAt(position, position + front, up);
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);

  double aspect_ratio = 1;
  glm::mat4 projection = glm::perspective(45.0, aspect_ratio, 0.1, 100.0);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
}

gboolean render(GtkGLArea *area, GdkGLContext *context) {

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw(program);

  gtk_gl_area_queue_render(area);
  return TRUE;
}

gboolean keypress_callback(GtkWidget *widget, GdkEventKey *event, gpointer data) {

  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

void button_callback(GtkButton *button, gpointer userData) {
  command *data=(command *)userData;
  printf("Hello, World! %d\n", data->movement_type);

  if(data->movement_type == START_DRILL) {
    const char *command = "M3 S100\r\n";
    int r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
  }
  if(data->movement_type == STOP_DRILL) {
    const char *command = "M3 S0\r\n";
    int r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
  }
}

void movement_callback(GtkButton *button, gpointer userData) {
  command *data=(command *)userData;

  float speed=200;

  if(data->movement_type == UP) {
    char command[256] = "G91";
    int r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
    sprintf(command, "G1 F%f X0 Y0 Z%f\r\n", speed, 10.f);
    r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
  }
  if(data->movement_type == DOWN) {
    char command[256] = "G91";
    int r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
    sprintf(command, "G1 F%f X0 Y0 Z%f\r\n", speed, -10.f);
    r = sp_blocking_write(main_port, command, strlen(command), 1000);
    printf("%d\n", r);
  }
}

int main(int argc, char *argv[]) {

  struct sp_port **port_list;

  enum sp_return result = sp_list_ports(&port_list);
  if (result != SP_OK) {
    fprintf(stderr, "sp_list_ports failed.\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; port_list[i] != NULL; i++) {
    struct sp_port *port = port_list[i];
    char *port_name = sp_get_port_name(port);

    printf("Found port: %s\n", port_name);

    if (strcmp(port_name, "/dev/ttyUSB0") == 0) {
      sp_open(port_list[i], SP_MODE_READ_WRITE);
      sp_set_baudrate(port_list[i], 112500);
      main_port = port_list[i];
      usleep(2 * 1000 * 1000);
    }
  }

  gtk_init(&argc, &argv);

  // Main window
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "CNC");

  // Drawing area
  GtkWidget *gl_area = gtk_gl_area_new();

  // Button 1
  GtkWidget *start_drill_button = gtk_button_new_with_label("Start Drill");
  command *start_drill_command=(command *)malloc(sizeof(command));
  start_drill_command->movement_type = START_DRILL;
  g_signal_connect(start_drill_button, "clicked", G_CALLBACK(button_callback), (gpointer) start_drill_command);
  gtk_widget_set_name(start_drill_button, "start_drill_button");

  // Button 2
  GtkWidget *stop_drill_button = gtk_button_new_with_label("Stop Drill");
  command *stop_drill_command=(command *)malloc(sizeof(command));
  stop_drill_command->movement_type = STOP_DRILL;
  g_signal_connect(stop_drill_button, "clicked", G_CALLBACK(button_callback), (gpointer) stop_drill_command);
  gtk_widget_set_name(stop_drill_button, "stop_drill_button");

  // Button 3
  GtkWidget *up_button = gtk_button_new_with_label("Up");
  command *up_command=(command *)malloc(sizeof(command));
  up_command->movement_type = UP;
  g_signal_connect(up_button, "clicked", G_CALLBACK(movement_callback), (gpointer) up_command);
  gtk_widget_set_name(up_button, "up_button");

  // Button 4
  GtkWidget *down_button = gtk_button_new_with_label("Down");
  command *down_command=(command *)malloc(sizeof(command));
  down_command->movement_type = DOWN;
  g_signal_connect(down_button, "clicked", G_CALLBACK(movement_callback), (gpointer) down_command);
  gtk_widget_set_name(down_button, "down_button");
  // CSS
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  // Box
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
  gtk_container_add(GTK_CONTAINER(window), box);
  gtk_container_add(GTK_CONTAINER(box), start_drill_button);
  gtk_container_add(GTK_CONTAINER(box), stop_drill_button);
  gtk_container_add(GTK_CONTAINER(box), up_button);
  gtk_container_add(GTK_CONTAINER(box), down_button);

  // Signals
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);

  // Main loop
  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_main();

  sp_free_port_list(port_list);
}
