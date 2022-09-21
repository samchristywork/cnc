#include <epoxy/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtk/gtk.h>
#include <libserialport.h>

GLuint buffer;
GLuint program;
glm::mat4 model = glm::mat4(1.0);
struct sp_port **port_list;
struct sp_port *main_port;

float dist_increment = 4;

enum MOVEMENT_TYPE {
  ABORT,
  START_DRILL,
  STOP_DRILL,
  XM,
  XP,
  YM,
  YP,
  ZM,
  ZP
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
  glGenBuffers(1, &buffer);

  glBindVertexArray(buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);

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

  glBindVertexArray(buffer);
  glDrawArrays(GL_TRIANGLES, 0, 36);

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

void ports_init() {
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
}

gboolean keypress_callback(GtkWidget *widget, GdkEventKey *event, gpointer data) {

  if (event->keyval == GDK_KEY_Escape) {
    char command[256];
    sprintf(command, "M00\r\n");
    sp_nonblocking_write(main_port, command, strlen(command));

    sp_free_port_list(port_list);

    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

void drill_speed_slider_callback(GtkRange *range, gpointer userData) {
  float drill_speed = gtk_range_get_value(range);
  char command[256];
  sprintf(command, "M3 S%f\r\n", drill_speed);
  int r = sp_nonblocking_write(main_port, command, strlen(command));
  printf("%d\n", r);
}

void movement_speed_slider_callback(GtkRange *range, gpointer userData) {
  dist_increment = gtk_range_get_value(range);
}

void button_callback(GtkButton *button, gpointer userData) {
  command *data = (command *)userData;

  {
    char command[256] = "G91\r\n";
    int r = sp_nonblocking_write(main_port, command, strlen(command));
    printf("%d\n", r);
  }

  char command[256];
  bzero(command, 256);

  float speed = 1000;

  switch (data->movement_type) {
  case ABORT:
    sp_close(main_port);
    sp_free_port_list(port_list);
    ports_init();
    break;
  case START_DRILL:
    sprintf(command, "M3 S%f\r\n", 100.f);
    break;
  case STOP_DRILL:
    sprintf(command, "M3 S%f\r\n", 0.f);
    break;
  case XP:
    sprintf(command, "G1 F%f X%f Y0 Z0\r\n", speed, dist_increment);
    break;
  case XM:
    sprintf(command, "G1 F%f X%f Y0 Z0\r\n", speed, -dist_increment);
    break;
  case YP:
    sprintf(command, "G1 F%f X0 Y%f Z0\r\n", speed, dist_increment);
    break;
  case YM:
    sprintf(command, "G1 F%f X0 Y%f Z0\r\n", speed, -dist_increment);
    break;
  case ZP:
    sprintf(command, "G1 F%f X0 Y0 Z%f\r\n", speed, dist_increment);
    break;
  case ZM:
    sprintf(command, "G1 F%f X0 Y0 Z%f\r\n", speed, -dist_increment);
    break;
  default:
    printf("Unrecognized movement type: %d\n", data->movement_type);
    break;
  }

  int r = sp_nonblocking_write(main_port, command, strlen(command));
  printf("%d\n", r);
}

void add_button(GtkWidget *box, const char *label, int movement_type, const char *name) {
  GtkWidget *button = gtk_button_new_with_label(label);
  command *c = (command *)malloc(sizeof(command));
  c->movement_type = movement_type;
  g_signal_connect(button, "clicked", G_CALLBACK(button_callback), (gpointer)c);
  gtk_widget_set_name(button, name);
  gtk_container_add(GTK_CONTAINER(box), button);
}

int main(int argc, char *argv[]) {

  ports_init();

  gtk_init(&argc, &argv);

  // Main window
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "CNC");

  // Boxes
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_container_add(GTK_CONTAINER(window), box);
  GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE);
  GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE);

  // Drawing area
  GtkWidget *gl_area = gtk_gl_area_new();
  gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);

  // Buttons
  add_button(box, "Start Drill", START_DRILL, "start_drill_button");
  add_button(box, "Stop Drill", STOP_DRILL, "stop_drill_button");
  gtk_container_add(GTK_CONTAINER(box), hbox1);
  add_button(hbox1, "Z- (Q)", ZM, "zm_button");
  add_button(hbox1, "Y+ (W)", YP, "yp_button");
  add_button(hbox1, "Z+ (E)", ZP, "zp_button");
  gtk_container_add(GTK_CONTAINER(box), hbox2);
  add_button(hbox2, "X- (A)", XM, "xm_button");
  add_button(hbox2, "Y- (S)", YM, "ym_button");
  add_button(hbox2, "X+ (D)", XP, "xp_button");

  // Sliders
  GtkWidget *drill_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 10000, .01);
  g_signal_connect(drill_speed_slider, "value-changed", G_CALLBACK(drill_speed_slider_callback), NULL);
  gtk_widget_set_name(drill_speed_slider, "drill_speed_slider");
  gtk_container_add(GTK_CONTAINER(box), drill_speed_slider);

  GtkWidget *movement_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 3, .001);
  g_signal_connect(movement_speed_slider, "value-changed", G_CALLBACK(movement_speed_slider_callback), NULL);
  gtk_widget_set_name(movement_speed_slider, "movement_speed_slider");
  gtk_container_add(GTK_CONTAINER(box), movement_speed_slider);

  // Abort
  add_button(box, "ABORT", ABORT, "abort_button");

  // CSS
  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  // Signals
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

  // Events
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);

  // Main loop
  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_main();
}
