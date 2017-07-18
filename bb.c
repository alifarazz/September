#include <gtk/gtk.h>
int check=0;
void on_compare_clicked(GtkWidget *button)
{
    g_print("compare clicked");
}
void on_del_clicked(GtkWidget *button,GtkListBox *box)
{
    GtkListBoxRow *listboxrow=gtk_list_box_get_selected_row(box);
    gtk_container_remove(GTK_CONTAINER(box),GTK_WIDGET(listboxrow));

}
void stop_pressed(GtkButton *button,GdkEventButton *event)
{
    
    if ((event->type==GDK_BUTTON_PRESS) && (check==0)) //check if pause or not play//
    {
	g_print("one clicked");
	check=1;
    }
    else if ((event->type==GDK_2BUTTON_PRESS) && (check == 1))//check if play //
    {
	g_print("double clicked");
	check=0;
    }

}
int main(int argc, char *argv[])
{
    GtkBuilder      *builder;
    GtkWidget       *window;
    GtkWidget       *popup;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "wer.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    gtk_builder_connect_signals(builder, NULL);

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    return 0;
}
G_MODULE_EXPORT void on_treeview1_row_activated()
{

    GDateTime *now = g_date_time_new_now_local();
    now=g_date_time_add_minutes (now,270);
    gchar *my_time = g_date_time_format(now, "%H:%M:%S %p");
    g_print("%s", my_time);
}
G_MODULE_EXPORT void on_button1_clicked(GtkButton *Button , GtkListBox *box)
{
  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *play;
  GtkWidget *compare;
  GtkWidget *name;
  GtkWidget *time;
  GtkWidget *created;
  GtkWidget *del;
  GtkWidget *eventbox;

  GtkWidget *imageplay;
  GtkWidget *imagecompare;
  GtkWidget *imagedel;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_list_box_insert(box,hbox,-1);

  eventbox=gtk_event_box_new();
  play = gtk_button_new();

  g_signal_connect(G_OBJECT(play),"button_press_event",
		G_CALLBACK(stop_pressed),(gpointer) eventbox);
  gtk_container_add(GTK_CONTAINER(eventbox),play);
  compare = gtk_button_new();
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  name = gtk_label_new("SOUND1");
  time=gtk_label_new("0.0/2.0");
  gtk_box_pack_start(GTK_BOX(vbox), name, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), time, TRUE, TRUE, 0);
  created=gtk_label_new("12:12:12 Am");
  
  del = gtk_button_new();

  imageplay=gtk_image_new_from_file ("rsz_1icon-play-128.png");
  gtk_button_set_image(GTK_BUTTON(play),imageplay);
  imagecompare=gtk_image_new_from_file ("rsz_1141961-200.png");
  gtk_button_set_image(GTK_BUTTON(compare),imagecompare);
  imagedel=gtk_image_new_from_file ("rsz_1trash_recyclebin_empty_closed.png");
  gtk_button_set_image(GTK_BUTTON(del),imagedel);


  gtk_box_pack_start(GTK_BOX(hbox),eventbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), compare, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), created, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), del, TRUE, TRUE, 50);


  g_signal_connect(G_OBJECT(compare), "clicked",
        G_CALLBACK(on_compare_clicked), NULL);

  g_signal_connect(G_OBJECT(del), "clicked",
        G_CALLBACK(on_del_clicked), (gpointer) box);
  g_object_ref(hbox);

  gtk_widget_show_all(hbox);

}
