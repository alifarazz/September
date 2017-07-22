#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#include <pthread.h>

#ifdef __linux__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include <gtk/gtk.h>

#include "./frame/frame.h"
#include "./gmm/gmm.h"
#include "./gmmp/gmmp.h"
#include "./mfcc/mfcc.h"
#include "./openAL/openAL.h"
#include "./x2x/x2x.h"

enum {
  MODEL=0,
  SCORE,
  PROGRESS,
  PROGRESSTXT,
  ABSPATH
};

G_MODULE_EXPORT void on_main_window_destory(GtkButton *button, gpointer app);

ALuint duration = 5;
GtkWidget *about_popup;

char* extractName (char *name)
{
  int len = strlen(name);
  while (name[len-1] != '/' && len)
    len--;
  return name + len;
}

static void
create_new_model_activated (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       app)
{
  GtkWidget *dialog;
  gchar *modelname;
  OALSampleSet sset;
  gint result;
  FILE *charfp, *x2xfp, *framefp, *mfccfp;

  sset.format = FORMAT;
  sset.freq   = SAMPLERATE;

  Record(&sset, duration);
  /* Play(&sset); */

  charfp = charFromsamp((char*) sset.data, sset.len); rewind(charfp); free(sset.data);
  x2xfp = x2xFromchar(charfp); rewind(x2xfp); fclose(charfp);
  framefp = frameFromX2x(x2xfp); rewind(framefp); fclose(x2xfp);
  mfccfp = MFCCFromFrame(framefp); rewind(mfccfp); fclose(framefp);

  dialog = gtk_file_chooser_dialog_new ("Save Model As ...", NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);
  result = gtk_dialog_run (GTK_DIALOG (dialog));
  if (result == GTK_RESPONSE_ACCEPT) {
    modelname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    gmmFromMFCC(modelname, mfccfp);

    g_print("created Model %s\n", modelname);
		g_free(modelname);
  } else
    g_print("model creation canceled\n");
  gtk_widget_destroy (dialog);

  fclose(mfccfp);
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       app)
{
  gtk_dialog_run(GTK_DIALOG(about_popup));
  gtk_widget_hide(about_popup);
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
  g_application_quit(app);
}

static GActionEntry app_entries[] = {
  /* { "pref", preferences_activated, NULL, NULL, NULL }, */
  /* { "about", about_activated, NULL, NULL, NULL}, */
  { "createModel", create_new_model_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};

static void
app_shutdown (GApplication *app,
              gpointer      user_data)
{
  DestroyState();
  g_print("bye...\n");
}

static void
app_activate (GApplication *app,
              gpointer      user_data)
{
  GtkWidget       *window;
  GtkBuilder      *builder;
  GMenu           *menu;
  builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "./glade/main_window.glade", NULL);

  window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  about_popup= GTK_WIDGET(gtk_builder_get_object( builder, "abdialog" ));
  gtk_builder_connect_signals(builder, NULL);
  g_signal_connect (G_OBJECT(window),
                    "destroy",
                    G_CALLBACK(on_main_window_destory),
                    G_OBJECT(app));

  gtk_application_add_window(app, window);
  gtk_application_add_window(app, about_popup);
  g_object_unref(builder);

  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries, G_N_ELEMENTS (app_entries), app);
  menu = g_menu_new ();
  g_menu_append (menu, "Create New Model", "app.createModel");
  g_menu_append (menu, "About", "app.about");
  g_menu_append (menu, "Quit", "app.quit");
  gtk_application_set_app_menu (G_APPLICATION (app), menu);
  g_object_unref(menu);

  gtk_application_window_set_show_menubar (window, FALSE);
  gtk_widget_show(window);
}

int main(int argc, char *argv[])
{
  GtkWidget *app;
  int status;

  InitState(SAMPLERATE, FORMAT, calcBufLen(SAMPLERATE, DURATION, FORMAT));
  /* alGetError(); /\* clear error state *\/ */

  /* gtk_init(&argc, &argv); */

  app = gtk_application_new("org.gnome.SPEAKER_RECOGNITION", G_APPLICATION_FLAGS_NONE);

  g_signal_connect(app, "shutdown", G_CALLBACK(app_shutdown), NULL);
  g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);

  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  /* gtk_widget_show(window); */
  /* gtk_main(); */

  return status;
}

G_MODULE_EXPORT void on_record_btn_clicked(GtkButton *Button ,GtkListStore *list)
{
  GtkTreeIter iter;
  gboolean valid;
  gint j;
  float result;

  OALSampleSet sset;
  FILE *charfp, *x2xfp, *framefp, *mfccfp;
  float min = FLT_MAX, max = -FLT_MAX;
  char resulttxt[10];
	char *filename;
  /* int c; */
  /* FILE *outputfpmfcc = fopen("./res1.mfcc", "wb"); */

  sset.format = FORMAT;
  sset.freq   = SAMPLERATE;

  Record(&sset, duration);
  /* Play(&sset); */

  charfp = charFromsamp((char*) sset.data, sset.len); rewind(charfp); free(sset.data);
  x2xfp = x2xFromchar(charfp); rewind(x2xfp); fclose(charfp);
  framefp = frameFromX2x(x2xfp); rewind(framefp); fclose(x2xfp);
  mfccfp = MFCCFromFrame(framefp); rewind(mfccfp); fclose(framefp);


  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list), &iter);
	while (valid) {
		gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, ABSPATH, &filename, -1);
		if(filename == NULL) {
			g_print("encounterd NULL filename\n");
			break;
		}

    /* g_print("Checked Against: %s\n",filename[j]); */
    result = gmmpFromMFCC(filename, mfccfp); rewind(mfccfp);

    if (max < result)
      max = result;
    if (min > result)
      min = result;

    g_print("%s: %f\n", extractName(filename), result);
    if (result +132 <= 100 && 0 <= result + 132)
      sprintf (resulttxt, "%d%%", (int) (result + 132));
    else if (result + 132 > 100)
      sprintf (resulttxt, "%d%%", 100);
    else if (result + 132 < 0)
      sprintf (resulttxt, "%d%%", 0);

    gtk_list_store_set(list, &iter,
											 /*MODEL, extractName(filename),*/
											 SCORE, result,
											 PROGRESS, atoi(resulttxt),
											 PROGRESSTXT, resulttxt,
											 -1);
		g_free(filename);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list), &iter);
  }

  g_print("min is %f\n", min);
  g_print("max is %f\n", max);

  fclose(mfccfp);
}

G_MODULE_EXPORT void on_rmModel_clicked(GtkButton *Button,GtkTreeView *view) {

  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeIter  iter;

  selection= gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
  model = gtk_tree_view_get_model(view);

  if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) {
    return;
  }

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_list_store_remove(store, &iter);
  }
}

G_MODULE_EXPORT void on_addModel_clicked(GtkButton *button,GtkListStore *list)
{
  GtkTreeIter iter;
  GtkWidget *dialog;
  GtkFileFilter *filter;
	char *filename;
	gfloat foo = 0;

  dialog=gtk_file_chooser_dialog_new("Select a Model",NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     GTK_STOCK_OK,GTK_RESPONSE_OK,
                                     GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,NULL);
  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*.gmm");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  gtk_widget_show_all(dialog);
  gint response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response==GTK_RESPONSE_OK){
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
											 MODEL, extractName(filename),
											 SCORE, foo,
                       PROGRESS, 0,
											 PROGRESSTXT, "",
											 ABSPATH, filename,
											 -1);
		g_print("filename added %s\n", filename);
		g_free(filename);
  } else
    g_print("canceled\n");

  gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT void on_adjustment_value_changed(GtkAdjustment *adjustment)
{
  duration = (ALuint)gtk_adjustment_get_value(adjustment);
}

G_MODULE_EXPORT void on_about_btn_clicked(GtkButton *button,GtkWidget *popup)
{
  gtk_dialog_run(GTK_DIALOG(popup));
  gtk_widget_hide(popup);
}

G_MODULE_EXPORT void on_main_window_destory(GtkButton *button, gpointer app)
{
  g_application_quit(app);
}
