#ifndef EXAMPLE_UI_H_
#define EXAMPLE_UI_H_

#include <glibmm/dispatcher.h>
#include <glog/logging.h>
#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stack.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

#include <iostream>

#include "base/utils.h"

class ClientShowVideoPage : public Gtk::Box {
 public:
  ClientShowVideoPage();
  ~ClientShowVideoPage() = default;

 protected:
  Gtk::Button button_disconnect_;
  Gtk::DrawingArea drawing_area_video_;
};

class ClientInputServerInfoPage : public Gtk::Box {
 public:
  ClientInputServerInfoPage();
  ClientInputServerInfoPage(Gtk::Stack* stack) : ClientInputServerInfoPage() {
    stack_ = stack;
  }
  ~ClientInputServerInfoPage() = default;

  std::string GetServerIP();
  uint16_t GetServerPort();
  avrtc::ClientType GetClientType();
  std::string GetFilename();

 protected:
  void OnClientTypeChanged();
  void OnFileButtonClicked();
  void OnConnectButtonClicked();

 private:
  Gtk::Stack* stack_;

  Gtk::Entry entry_server_ip_;
  Gtk::Entry entry_server_port_;
  Gtk::ComboBoxText comboBoxText_client_type_;
  Gtk::Box file_chooser_;
  Gtk::Button button_choose_file_;
  Gtk::Label label_chosen_file_;
};

class ClientUI : public Gtk::Window {
 public:
  ClientUI();
  ~ClientUI() = default;

  void Show();

 private:
  Gtk::Stack m_Stack_;
  ClientInputServerInfoPage clientInputServerInfoPage;
  ClientShowVideoPage clientShowVideoPage;
};

class ModelColumns : public Gtk::TreeModel::ColumnRecord {
 public:
  ModelColumns() {
    add(m_col_id);
    add(m_col_name);
  }

  Gtk::TreeModelColumn<int> m_col_id;
  Gtk::TreeModelColumn<std::string> m_col_name;
};

class ServerUI : public Gtk::Window {
 public:
  ServerUI();
  ~ServerUI() = default;

  void OnSelectNewSender();

  void AddSender(const std::string& sender_name, int id);
  void RemoveSender(int id);
  void AddReceiver(const std::string& receiver_name, int id);
  void RemoveReceiver(int id);
  void SetSelectedSender(int id);
  std::function<void()> PopTask();
  void PushTask(std::function<void()> task);
  void Emit();

 protected:
  void AddClientSection_(Glib::RefPtr<Gtk::ListStore> refListStore,
                         const std::string& sender_name,
                         int id);
  void RemoveClientSection_(Glib::RefPtr<Gtk::ListStore> refListStore, int id);
  Gtk::Box box_;

  ModelColumns m_Columns;

  Glib::RefPtr<Gtk::ListStore> sender_refListStore;
  Gtk::TreeView sender_treeview_;
  Gtk::ScrolledWindow sender_ScrolledWindow;

  Glib::RefPtr<Gtk::ListStore> receiver_refListStore;
  Gtk::TreeView receiver_treeview_;
  Gtk::ScrolledWindow receiver_ScrolledWindow;

  Glib::Dispatcher ui_dispatcher;

  std::mutex mutex_;
  std::vector<std::function<void()>> pending_ui_tasks_;
};

#endif  // EXAMPLE_UI_H_