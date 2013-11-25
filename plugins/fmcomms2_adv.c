/**
 * Copyright (C) 2013 Analog Devices, Inc.
 *
 * Licensed under the GPL-2.
 *
 **/
#include <stdio.h>

#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <glib/gthread.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <malloc.h>
#include <values.h>
#include <sys/stat.h>
#include <string.h>

#include "../osc.h"
#include "../iio_utils.h"
#include "../osc_plugin.h"
#include "../config.h"
#include "../iio_widget.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static gint this_page;
static GtkNotebook *nbook;

enum fmcomms2adv_wtype {
	CHECKBOX,
	SPINBUTTON,
	COMBOBOX,
	BUTTON,
};

struct w_info {
	enum fmcomms2adv_wtype type;
	char name[60];
};

static char dir_name[512];

static struct w_info attrs[] = {
	{CHECKBOX, "adi,2rx-2tx-mode-enable"},
	{SPINBUTTON, "adi,agc-adc-large-overload-exceed-counter"},
	{SPINBUTTON, "adi,agc-adc-large-overload-inc-steps"},
	{CHECKBOX, "adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable"},
	{SPINBUTTON, "adi,agc-adc-small-overload-exceed-counter"},
	{SPINBUTTON, "adi,agc-attack-delay-extra-margin-us"},
	{SPINBUTTON, "adi,agc-dig-gain-step-size"},
	{SPINBUTTON, "adi,agc-dig-saturation-exceed-counter"},
	{SPINBUTTON, "adi,agc-gain-update-interval-us"},
	{CHECKBOX, "adi,agc-immed-gain-change-if-large-adc-overload-enable"},
	{CHECKBOX, "adi,agc-immed-gain-change-if-large-lmt-overload-enable"},
	{SPINBUTTON, "adi,agc-inner-thresh-high"},
	{SPINBUTTON, "adi,agc-inner-thresh-high-dec-steps"},
	{SPINBUTTON, "adi,agc-inner-thresh-low"},
	{SPINBUTTON, "adi,agc-inner-thresh-low-inc-steps"},
	{SPINBUTTON, "adi,agc-lmt-overload-large-exceed-counter"},
	{SPINBUTTON, "adi,agc-lmt-overload-large-inc-steps"},
	{SPINBUTTON, "adi,agc-lmt-overload-small-exceed-counter"},
	{SPINBUTTON, "adi,agc-outer-thresh-high"},
	{SPINBUTTON, "adi,agc-outer-thresh-high-dec-steps"},
	{SPINBUTTON, "adi,agc-outer-thresh-low"},
	{SPINBUTTON, "adi,agc-outer-thresh-low-inc-steps"},
	{CHECKBOX, "adi,agc-sync-for-gain-counter-enable"},
	{SPINBUTTON, "adi,aux-adc-decimation"},
	{SPINBUTTON, "adi,aux-adc-rate"},
	{COMBOBOX, "adi,clk-output-mode-select"},
	{SPINBUTTON, "adi,ctrl-outs-enable-mask"},
	{SPINBUTTON, "adi,ctrl-outs-index"},
	{SPINBUTTON, "adi,dc-offset-attenuation-high-range"},
	{SPINBUTTON, "adi,dc-offset-attenuation-low-range"},
	{SPINBUTTON, "adi,dc-offset-count-high-range"},
	{SPINBUTTON, "adi,dc-offset-count-low-range"},
	{SPINBUTTON, "adi,dc-offset-tracking-update-event-mask"},
	{SPINBUTTON, "adi,elna-bypass-loss-mdB"},
	{SPINBUTTON, "adi,elna-gain-mdB"},
	{CHECKBOX, "adi,elna-rx1-gpo0-control-enable"},
	{CHECKBOX, "adi,elna-rx2-gpo1-control-enable"},
	{SPINBUTTON, "adi,elna-settling-delay-ns"},
	{CHECKBOX, "adi,ensm-enable-pin-pulse-mode-enable"},
	{CHECKBOX, "adi,ensm-enable-txnrx-control-enable"},
	{CHECKBOX, "adi,external-rx-lo-enable"},
	{CHECKBOX, "adi,external-tx-lo-enable"},
	{CHECKBOX, "adi,frequency-division-duplex-mode-enable"},
	{SPINBUTTON, "adi,gc-adc-large-overload-thresh"},
	{SPINBUTTON, "adi,gc-adc-ovr-sample-size"},
	{SPINBUTTON, "adi,gc-adc-small-overload-thresh"},
	{SPINBUTTON, "adi,gc-dec-pow-measurement-duration"},
	{CHECKBOX, "adi,gc-dig-gain-enable"},
	{SPINBUTTON, "adi,gc-lmt-overload-high-thresh"},
	{SPINBUTTON, "adi,gc-lmt-overload-low-thresh"},
	{SPINBUTTON, "adi,gc-low-power-thresh"},
	{SPINBUTTON, "adi,gc-max-dig-gain"},
	{COMBOBOX, "adi,gc-rx1-mode"},
	{COMBOBOX, "adi,gc-rx2-mode"},
	{SPINBUTTON, "adi,mgc-dec-gain-step"},
	{SPINBUTTON, "adi,mgc-inc-gain-step"},
	{CHECKBOX, "adi,mgc-rx1-ctrl-inp-enable"},
	{CHECKBOX, "adi,mgc-rx2-ctrl-inp-enable"},
	{COMBOBOX, "adi,mgc-split-table-ctrl-inp-gain-mode"},
	{SPINBUTTON, "adi,rssi-delay"},
	{SPINBUTTON, "adi,rssi-duration"},
	{COMBOBOX, "adi,rssi-restart-mode"},
	{SPINBUTTON, "adi,rssi-wait"},
	{COMBOBOX, "adi,rx-rf-port-input-select"},
	{COMBOBOX, "adi,split-gain-table-mode-enable"},
	{CHECKBOX, "adi,tdd-skip-vco-cal-enable"},
	{CHECKBOX, "adi,tdd-use-dual-synth-mode-enable"},
	{CHECKBOX, "adi,tdd-use-fdd-vco-tables-enable"},
	{SPINBUTTON, "adi,temp-sense-decimation"},
	{SPINBUTTON, "adi,temp-sense-measurement-interval-ms"},
	{SPINBUTTON, "adi,temp-sense-offset-signed"},
	{CHECKBOX, "adi,temp-sense-periodic-measurement-enable"},
	{COMBOBOX, "adi,tx-rf-port-input-select"},
	{CHECKBOX, "adi,update-tx-gain-in-alert-enable"},
	{CHECKBOX, "adi,xo-disable-use-ext-refclk-enable"},
	{COMBOBOX, "bist_prbs"},
	{COMBOBOX, "loopback"},
	{BUTTON, "initialize"},
};

void signal_handler_cb (GtkWidget *widget, gpointer data)
{
	struct w_info *item = data;
	unsigned val;
	char temp[40];

	switch (item->type) {
		case CHECKBOX:
			val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
			break;
		case BUTTON:
			val = 1;
			break;
		case SPINBUTTON:
			val = (unsigned) gtk_spin_button_get_value(GTK_SPIN_BUTTON (widget));
			break;
		case COMBOBOX:
			val = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
			break;
	}

	sprintf(temp, "%d\n", val);
	write_sysfs_string(item->name, dir_name, temp);
}

static void connect_widget(GtkBuilder *builder, struct w_info *item)
{
	GtkWidget *widget;
	char *signal = NULL;
	int val;

	widget = GTK_WIDGET(gtk_builder_get_object(builder, item->name));
	val = read_sysfs_posint(item->name, dir_name);

	/* check for errors, in case there is a kernel <-> userspace mismatch */
	if (val < 0) {
		printf("%s:%s: error accessing '%s' (%s)\n",
			__FILE__, __func__, item->name, strerror(-val));
		gtk_widget_hide(widget);
		return;
	}

	switch (item->type) {
		case CHECKBOX:
			signal = "toggled";
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !!val);
			break;
		case BUTTON:
			signal = "clicked";
			break;
		case SPINBUTTON:
			signal = "value-changed";
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), val);
			break;
		case COMBOBOX:
			signal = "changed";
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), val);
			break;
	}

	g_builder_connect_signal(builder, item->name, signal,
		G_CALLBACK(signal_handler_cb), item);
}

static int fmcomms2adv_init(GtkWidget *notebook)
{
	GtkBuilder *builder;
	GtkWidget *fmcomms2adv_panel;
	int i;

	builder = gtk_builder_new();
	nbook = GTK_NOTEBOOK(notebook);

	if (!gtk_builder_add_from_file(builder, "fmcomms2_adv.glade", NULL))
		gtk_builder_add_from_file(builder, OSC_GLADE_FILE_PATH "fmcomms2_adv.glade", NULL);

	fmcomms2adv_panel = GTK_WIDGET(gtk_builder_get_object(builder, "fmcomms2adv_panel"));

	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		connect_widget(builder, &attrs[i]);


	this_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), fmcomms2adv_panel, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), fmcomms2adv_panel, "FMComms2 Advanced");

	return 0;
}

static bool fmcomms2adv_identify(void)
{
	bool ret = !set_debugfs_paths("ad9361-phy");
	strcpy(dir_name, debug_name_dir());
	return ret;
}

const struct osc_plugin plugin = {
	.name = "FMComms2 Advanced",
	.identify = fmcomms2adv_identify,
	.init = fmcomms2adv_init,
};