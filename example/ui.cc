#include <example/ui.h>

ClientUI::ClientUI() : Gtk::Window() {
    set_default_size(800, 500);
    set_title("Client Window");
    add(m_Stack_);
    m_Stack_.set_transition_type(
        Gtk::StackTransitionType::STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    m_Stack_.add(clientInputServerInfoPage, "client_input_info_page",
                 "Client Input Info");
    m_Stack_.add(clientShowVideoPage, "client_show_video_page",
                 "Client Show Video");
    m_Stack_.set_visible_child("client_input_info_page");
    Show();
}

void ClientUI::Show() {
    show_all();
}

ClientInputServerInfoPage::ClientInputServerInfoPage()
    : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 10) {
    auto label_ip = Gtk::make_managed<Gtk::Label>("Server IP:");
    auto label_port = Gtk::make_managed<Gtk::Label>("Server Port:");
    auto label_client_type = Gtk::make_managed<Gtk::Label>("Client Type:");
    auto connect_button = Gtk::make_managed<Gtk::Button>("Connect");

    entry_server_ip_.set_text("127.0.0.1");
    entry_server_ip_.set_width_chars(15);
    entry_server_port_.set_text("14562");
    entry_server_port_.set_width_chars(8);

    comboBoxText_client_type_.append("Sender");
    comboBoxText_client_type_.append("Receiver");
    comboBoxText_client_type_.set_active_text("Sender");
    comboBoxText_client_type_.signal_changed().connect(
        sigc::mem_fun(*this, &ClientInputServerInfoPage::OnClientTypeChanged));

    connect_button->signal_clicked().connect(sigc::mem_fun(
        *this, &ClientInputServerInfoPage::OnConnectButtonClicked));

    auto box_ip_port = Gtk::make_managed<Gtk::Box>(
        Gtk::Orientation::ORIENTATION_HORIZONTAL, 10);
    box_ip_port->pack_start(*label_ip, Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_start(entry_server_ip_, Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_start(*label_port, Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_start(entry_server_port_, Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_start(*label_client_type, Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_start(comboBoxText_client_type_,
                            Gtk::PackOptions::PACK_SHRINK);
    box_ip_port->pack_end(*connect_button, Gtk::PackOptions::PACK_SHRINK);

    pack_start(*box_ip_port, Gtk::PackOptions::PACK_SHRINK);

    label_chosen_file_.set_text("");
    button_choose_file_.set_label("Choose File");
    button_choose_file_.signal_clicked().connect(
        sigc::mem_fun(*this, &ClientInputServerInfoPage::OnFileButtonClicked));

    auto label_file = Gtk::make_managed<Gtk::Label>("File path: ");
    file_chooser_.set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    file_chooser_.pack_start(*label_file, Gtk::PackOptions::PACK_SHRINK);
    file_chooser_.pack_start(label_chosen_file_, Gtk::PackOptions::PACK_SHRINK);
    file_chooser_.pack_end(button_choose_file_, Gtk::PackOptions::PACK_SHRINK);
    pack_start(file_chooser_, Gtk::PackOptions::PACK_SHRINK);

    set_margin_top(20);
    set_margin_bottom(20);
    set_margin_start(20);
    set_margin_end(20);
}

std::string ClientInputServerInfoPage::GetServerIP() {
    return entry_server_ip_.get_text();
}

uint16_t ClientInputServerInfoPage::GetServerPort() {
    return static_cast<uint16_t>(std::stoi(entry_server_port_.get_text()));
}

avrtc::ClientType ClientInputServerInfoPage::GetClientType() {
    std::string type = comboBoxText_client_type_.get_active_text();
    return type == "Sender" ? avrtc::ClientType::kSender
                            : avrtc::ClientType::kReceiver;
}

std::string ClientInputServerInfoPage::GetFilename() {
    return label_chosen_file_.get_text();
}

void ClientInputServerInfoPage::OnClientTypeChanged() {
    if (comboBoxText_client_type_.get_active_text() == "Sender")
        pack_start(file_chooser_, Gtk::PackOptions::PACK_SHRINK);
    else
        remove(file_chooser_);
}

void ClientInputServerInfoPage::OnFileButtonClicked() {
    Gtk::FileChooserDialog dialog(
        "Please choose a file",
        Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::ResponseType::RESPONSE_OK);

    int result = dialog.run();

    if (result == Gtk::ResponseType::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        label_chosen_file_.set_text(filename);
    }
}

void ClientInputServerInfoPage::OnConnectButtonClicked() {
    // std::string server_ip = GetServerIP();
    // uint16_t server_port = GetServerPort();
    // avrtc::ClientType client_type = GetClientType();
    // std::string filename = GetFilename();
    stack_->set_visible_child("client_show_video_page");
}

ClientShowVideoPage::ClientShowVideoPage() {}

ServerUI::ServerUI()
    : Gtk::Window(), box_(Gtk::Orientation::ORIENTATION_VERTICAL, 0) {
    set_default_size(800, 500);
    set_title("Server Window");
    add(box_);

    sender_refListStore = Gtk::ListStore::create(m_Columns);
    sender_treeview_.set_model(sender_refListStore);
    sender_treeview_.append_column("Sender Name", m_Columns.m_col_name);
    sender_treeview_.append_column("Sender ID", m_Columns.m_col_id);
    sender_ScrolledWindow.add(sender_treeview_);
    sender_treeview_.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &ServerUI::OnSelectNewSender));

    receiver_refListStore = Gtk::ListStore::create(m_Columns);
    receiver_treeview_.set_model(receiver_refListStore);
    receiver_treeview_.append_column("Receiver Name", m_Columns.m_col_name);
    receiver_treeview_.append_column("Receiver ID", m_Columns.m_col_id);
    receiver_ScrolledWindow.add(receiver_treeview_);
    receiver_treeview_.get_selection()->set_mode(
        Gtk::SelectionMode::SELECTION_NONE);

    box_.pack_start(sender_ScrolledWindow);
    box_.pack_start(receiver_ScrolledWindow);

    show_all();

    ui_dispatcher.connect([this]() {
        while (auto task = PopTask()) {
            task();
        }
    });
}

void ServerUI::AddClientSection_(Glib::RefPtr<Gtk::ListStore> refListStore,
                                 const std::string& sender_name,
                                 int id) {
    Gtk::TreeRow row = *(refListStore->append());
    row[m_Columns.m_col_id] = id;
    row[m_Columns.m_col_name] = sender_name;
}

void ServerUI::RemoveClientSection_(Glib::RefPtr<Gtk::ListStore> refListStore,
                                    int id) {
    auto children = refListStore->children();
    for (auto iter = children.begin(); iter != children.end(); ++iter) {
        auto row = *iter;
        if (row[m_Columns.m_col_id] == id) {
            refListStore->erase(iter);
            break;
        }
    }
}

void ServerUI::AddSender(const std::string& sender_name, int id) {
    AddClientSection_(sender_refListStore, sender_name, id);
}

void ServerUI::RemoveSender(int id) {
    RemoveClientSection_(sender_refListStore, id);
}

void ServerUI::AddReceiver(const std::string& receiver_name, int id) {
    AddClientSection_(receiver_refListStore, receiver_name, id);
}

void ServerUI::RemoveReceiver(int id) {
    RemoveClientSection_(receiver_refListStore, id);
}

void ServerUI::SetSelectedSender(int id) {
    auto children = sender_refListStore->children();
    for (auto iter = children.begin(); iter != children.end(); ++iter) {
        auto row = *iter;
        if (row[m_Columns.m_col_id] == id) {
            sender_treeview_.get_selection()->select(iter);
            break;
        }
    }
}

void ServerUI::OnSelectNewSender() {
    auto iter = sender_treeview_.get_selection()->get_selected();
    if (iter) {
        int id = (*iter)[m_Columns.m_col_id];
        std::string name = (*iter)[m_Columns.m_col_name];
        std::printf("Selected new sender: %s, ID: %d\n", name.c_str(), id);
    }
}

std::function<void()> ServerUI::PopTask() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_ui_tasks_.empty()) {
        return nullptr;
    }
    auto task = pending_ui_tasks_.front();
    pending_ui_tasks_.erase(pending_ui_tasks_.begin());
    return task;
}

void ServerUI::PushTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_ui_tasks_.push_back(task);
}

void ServerUI::Emit() {
    ui_dispatcher.emit();
}