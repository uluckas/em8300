// Prototypes for the casllback functions
#ifndef __CALLBACK__
#define __CALLBACK__

#include <pthread.h>
#include <gtk/gtk.h>

extern int dev;
struct state {
	int tvmode, aspect, audio, spu, bcs;
} cur_state;	

em8300_bcs_t bcs;
pthread_t bcs_th;
pthread_mutex_t mut; 
pthread_cond_t cond; 

gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);
gint set_audio (GtkWidget *widget, GdkEvent *event, gpointer data);
gint set_tvmode (GtkWidget *widget, GdkEvent *event, gpointer data);
gint set_aspect (GtkWidget *widget, GdkEvent *event, gpointer data);
gint set_spu (GtkWidget *widget, GdkEvent *event, gpointer data);
gint destroy (GtkWidget *widget, GdkEvent *event, gpointer data);
gint press (GtkWidget *widget, GdkEventButton *event, gpointer data);
gint motion (GtkWidget *widget, GdkEventMotion *event, gpointer data);
gint release (GtkWidget *widget, GdkEventButton *event, gpointer data);
gint client_e (GtkWidget *widget, GdkEventClient *event, gpointer data);
gint slider_move (GtkWidget *widget, GdkEventButton *event, gpointer data);
gint slider_press (GtkWidget *widget, GdkEventButton *event, gpointer data);
gint slider_release (GtkWidget *widget, GdkEventButton *event, gpointer data);
gint bcs_toggle (GtkWidget *widget, GdkEvent *event, gpointer data);
void  *bcs_thread (void *arg);

#endif
