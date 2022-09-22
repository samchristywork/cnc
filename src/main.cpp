#include "raylib.h"
#include <gtk/gtk.h>
#include <math.h>
#include <libserialport.h>

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

void die() {
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  pid_t pid = fork();
  if (pid < 0) {
    die();
  }
  if (pid) {

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(0, 0, "CNC");

    Camera camera = {0};
    camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Model model1 = LoadModel("models/collet.obj");
    Model model2 = LoadModel("models/bit.obj");
    Model model3 = LoadModel("models/chuck.obj");

    //Texture2D texture = LoadTexture("untitled.png");
    //model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    SetCameraMode(camera, CAMERA_FREE);

    SetTargetFPS(60);

    Vector3 cubePosition = {0.0f, 0.0f, 0.0f};
    Vector3 cubeSize = {1.0f, 1.0f, 1.0f};
    Vector2 cubeScreenPosition = {0.0f, 0.0f};

    Ray ray = {0};

    RayCollision collision = {0};

    float angle=0;

    while (!WindowShouldClose()) {
      UpdateCamera(&camera);
      if (IsKeyDown('Q')) {
        camera.target.y-=.1;
      }
      if (IsKeyDown('E')) {
        camera.target.y+=.1;
      }
      if (IsKeyDown('D')) {
        angle+=.1;
      }
      camera.position.x=10.f*sin(angle);
      camera.position.z=10.f*cos(angle);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!collision.hit) {
          ray = GetMouseRay(GetMousePosition(), camera);

          collision = GetRayCollisionBox(ray,
                                         (BoundingBox){(Vector3){cubePosition.x - cubeSize.x / 2, cubePosition.y - cubeSize.y / 2, cubePosition.z - cubeSize.z / 2},
                                                       (Vector3){cubePosition.x + cubeSize.x / 2, cubePosition.y + cubeSize.y / 2, cubePosition.z + cubeSize.z / 2}});
        } else
          collision.hit = false;
      }
      cubeScreenPosition = GetWorldToScreen((Vector3){cubePosition.x, cubePosition.y + 2.5f, cubePosition.z}, camera);
      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawFPS(10, 10);
      BeginMode3D(camera);

      DrawModelEx(model1, {0,0,0}, {1,0,0}, 90, {4,4,4}, CLITERAL(Color){200,100,100,255});
      DrawModelWiresEx(model1, {0,0,0}, {1,0,0}, 90, {4,4,4}, CLITERAL(Color){130,30,30,255});
      DrawModelEx(model2, {0,0,0}, {1,0,0}, 0, {1,1,1}, CLITERAL(Color){100,200,100,255});
      DrawModelWiresEx(model2, {0,0,0}, {1,0,0}, 0, {1,1,1}, CLITERAL(Color){30,130,30,255});
      DrawModelEx(model3, {0,0,0}, {0,-1,0}, 90, {.75,.75,.75}, CLITERAL(Color){100,100,200,255});
      DrawModelWiresEx(model3, {0,0,0}, {0,-1,0}, 90, {.75,.75,.75}, CLITERAL(Color){30,30,130,255});

      DrawGrid(100, 1.0f);

      EndMode3D();
      DrawText("Enemy: 100 / 100", (int)cubeScreenPosition.x - MeasureText("Enemy: 100/100", 20) / 2, (int)cubeScreenPosition.y, 20, BLACK);
      if (collision.hit)
        DrawText("BOX SELECTED", 10, 10, 30, GREEN);
      EndDrawing();
    }

    CloseWindow();
  } else {
    usleep(1000 * 1000);

    ports_init();

    gtk_init(&argc, &argv);

    // Main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), false);
    gtk_window_set_title(GTK_WINDOW(window), "CNC");

    // Boxes
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
    gtk_container_add(GTK_CONTAINER(window), box);
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE);
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE);

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

    // Events
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(keypress_callback), NULL);

    char asdf[256];
    sp_blocking_read(main_port, asdf, 255, 1000);
    char command[256] = "M3 S100.\r\n";
    sp_blocking_write(main_port, command, strlen(command), 1000);
    puts(command);
    bzero(command, 256);
    sp_blocking_read(main_port, command, 255, 1000);
    puts(command);

    // Main loop
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_main();
  }
}
