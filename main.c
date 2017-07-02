#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#include <pthread.h>

#ifdef __linux__
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alut.h>
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

gint i=0;
ALuint duration = 5;

char* extractName (char *name)
{
  int len = strlen(name);
  while (name[len-1] != '/' && len)
    len--;
  return name + len;
}

int main(int argc, char *argv[])
{
  GtkBuilder      *builder;
  GtkWidget       *window;
  GtkWidget       *popup;

  InitState(SAMPLERATE, FORMAT, calcBufLen(SAMPLERATE, DURATION, FORMAT));
  alGetError(); /* clear error state */

  gtk_init(&argc, &argv);

  builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "./glade/window_main.glade", NULL);

  window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  popup  = GTK_WIDGET(gtk_builder_get_object( builder, "abdialog" ));
  gtk_builder_connect_signals(builder, NULL);

  g_object_unref(builder);

  gtk_widget_show(window);
  gtk_main();

  return 0;
}

G_MODULE_EXPORT void on_record_gmmp_btn_clicked(GtkButton *Button ,GtkListStore *list)
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
    sprintf (resulttxt, "%d", (int) (result + 132));
    gtk_list_store_set(list, &iter,
											 /*MODEL, extractName(filename),*/
											 SCORE, result,
											 PROGRESS, (int) (result + 132),
											 PROGRESSTXT, resulttxt,
											 -1);
		g_free(filename);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list), &iter);
  }

  g_print("min is %f\n", min);
  g_print("max is %f\n", max);

  fclose(mfccfp);
}

G_MODULE_EXPORT void on_record_createModel_btn_clicked(GtkButton *Button)
{
  GtkWidget *dialog;
  gchar *modelname;
  OALSampleSet sset;
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
  gint result = gtk_dialog_run (GTK_DIALOG (dialog));
  if (result == GTK_RESPONSE_ACCEPT) {
    modelname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    gmmFromMFCC(modelname, mfccfp);

    g_print("created Model %s\n", modelname);
		g_free(modelname);
  } else
    g_print("model canceled\n");
  gtk_widget_destroy (dialog);

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
    g_print("filename added %s\n", filename);
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

G_MODULE_EXPORT void on_main_window_destroy()
{
  DestroyState();
  gtk_main_quit();
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
