/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    upnp_scanner_dlg.cpp
 * Authors: Björn Petersen
 * Purpose: Using UPNP/DLNA devices
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_UPNP
#include <sjmodules/scanner/upnp_scanner.h>
#include <sjmodules/scanner/upnp_scanner_dlg.h>
#include <sjtools/msgbox.h>


SjUpnpDialog::SjUpnpDialog (wxWindow* parent, SjUpnpScannerModule* upnpModule, SjUpnpSource* upnpSource)
	: SjDialog(parent, "", SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	m_upnpModule   = upnpModule;
	m_upnpSource   = upnpSource; // may be NULL!
	m_isNew        = (upnpSource==NULL);
	m_stillLoading = true;
	m_dirListFor   = NULL;

	if( m_isNew ) {
		SetTitle(_("Add an UPnP/DLNA server"));
	}
	else {
		wxString::Format(_("Options for \"%s\""), upnpSource->GetDisplayUrl().c_str());
	}


	// create dialog
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

		wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxALL|wxGROW, SJ_DLG_SPACE);

			wxStaticText* staticText = new wxStaticText(this, -1, "1. "+_("Select server:"));
			sizer2->Add(staticText, 1, 0, SJ_DLG_SPACE);

			m_stillScanningText = new wxStaticText(this, -1, _("(still scanning)"));
			sizer2->Add(m_stillScanningText, 0, 0, SJ_DLG_SPACE);

		m_mediaServerListCtrl = new wxListCtrl(this, IDC_MEDIASERVERLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_mediaServerListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_mediaServerListCtrl->InsertColumn(0, _("Name"));
		sizer1->Add(m_mediaServerListCtrl, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		staticText = new wxStaticText(this, -1, "2. "+_("Select directory:"));
		sizer1->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		m_dirListCtrl = new wxListCtrl(this, IDC_DIRLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_dirListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_dirListCtrl->InsertColumn(0, _("Directory"));
		sizer1->Add(m_dirListCtrl, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// init done, center dialog
	UpdateMediaServerList();
	sizer1->SetSizeHints(this);
	CentreOnParent();
}


SjUpnpMediaServer* SjUpnpDialog::GetSelectedMediaServer()
{
	SjUpnpMediaServer* selMediaServer = NULL;
	long selIndex = GetSelListCtrlItem(m_mediaServerListCtrl);
	if( selIndex >= 0 ) { selMediaServer = (SjUpnpMediaServer*)m_mediaServerListCtrl->GetItemData(selIndex); }
	return selMediaServer;
}


SjUpnpDirEntry* SjUpnpDialog::GetSelectedDirEntry()
{
	long selIndex = GetSelListCtrlItem(m_dirListCtrl);
	if( selIndex >= 0 && selIndex < m_dirListCtrl->GetItemCount() )
	{
		selIndex = m_dirListCtrl->GetItemData(selIndex);
		if( selIndex == -1 ) {
			if( m_parents.GetCount() <= 0 ) { return NULL; } // error
			m_parentDirEntry.m_id = m_parents.Last();
			m_parents.RemoveAt(m_parents.GetCount()-1);
			return &m_parentDirEntry;
		}
		else if( selIndex >= 0 && selIndex < m_currDir.GetCount() ) {
			return m_currDir.Item(selIndex);
		}
	}

	return NULL;
}


void SjUpnpDialog::UpdateMediaServerList()
{
	wxCriticalSectionLocker locker(m_upnpModule->m_mediaServerCritical);

	SjUpnpMediaServer* selMediaServer = GetSelectedMediaServer();

	m_mediaServerListCtrl->DeleteAllItems();

	SjHashIterator     iterator;
	wxString           udn;
	SjUpnpMediaServer* mediaServer;
	int i = 0;
	while( (mediaServer=(SjUpnpMediaServer*)m_upnpModule->m_mediaServerList.Iterate(iterator, udn))!=NULL ) {
		wxListItem li;
		li.SetId(i++);
		li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
		li.SetText(mediaServer->_friendly_name);
		li.SetImage(SJ_ICON_INTERNET_SERVER);
		li.SetData((void*)mediaServer);
		int new_i = m_mediaServerListCtrl->InsertItem(li);
		if( mediaServer == selMediaServer ) {
			m_mediaServerListCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
		}
	}
}


void SjUpnpDialog::UpdateDirList()
{
	m_dirListCtrl->DeleteAllItems();

	long zero_based_pos = 0;
	if( m_currDir.getObjectID()!="0" ) {
		wxListItem li;
		li.SetId(zero_based_pos++);
		li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
		li.SetText("..");
		li.SetImage(SJ_ICON_EMPTY);
		li.SetData(-1);
		m_dirListCtrl->InsertItem(li);
	}

	int i, cnt = m_currDir.GetCount();
	for( i = 0; i < cnt; i++ )
	{
		SjUpnpDirEntry* entry = m_currDir.Item(i);

		wxListItem li;
		li.SetId(zero_based_pos++);
		li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
		li.SetText(entry->m_name);
		li.SetImage(entry->m_isDir? SJ_ICON_MUSIC_FOLDER : SJ_ICON_EMPTY);
		li.SetData(i);
		m_dirListCtrl->InsertItem(li);
	}
}


void SjUpnpDialog::OnUpdateMediaServerList(wxCommandEvent&)
{
	UpdateMediaServerList();
}


void SjUpnpDialog::OnScanDone(wxCommandEvent&)
{
	m_stillScanningText->Hide();
}


void SjUpnpDialog::OnSize(wxSizeEvent& e)
{
	wxSize size = m_mediaServerListCtrl->GetClientSize();
	m_mediaServerListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);

	size = m_dirListCtrl->GetClientSize();
	m_dirListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);
	e.Skip();
}


void SjUpnpDialog::OnMediaServerClick(wxListEvent&)
{
    SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
    if( mediaServer == NULL ) { return; } // nothing selected
    if( mediaServer == m_dirListFor ) { return; } // already selected
	m_dirListFor = mediaServer;

	wxBusyCursor busy;

	m_currDir.setObjectID("0");
    mediaServer->fetchContents(m_currDir);

    UpdateDirList();
}


void SjUpnpDialog::OnMediaServerContextMenu(wxListEvent&)
{
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    bool sthSelected = GetSelectedMediaServer()!=NULL;

    SjMenu m(0);
    m.Append(IDC_MEDIASERVERINFO, _("Info..."));
    m.Enable(IDC_MEDIASERVERINFO, sthSelected);

    PopupMenu(&m, pt);
}


void SjUpnpDialog::OnMediaServerInfo(wxCommandEvent&)
{
	SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
	if( mediaServer == NULL ) { return; } // nothing selected

	wxString subscriptionId(mediaServer->_subscription_id, sizeof(Upnp_SID));
	wxMessageBox(
		wxString::Format(
			"UDN: %s\n\nfriendlyName: %s\n\nmodelDescription: %s\n\nmanufacturer: %s\n\ndeviceType: %s\n\n"
			"ContentDirectory.eventSubURL: %s\n\nContentDirectory.controlURL: %s\n\nContentDirectory.serviceType: %i\n\n"
			"Subscription-ID: %s\n\nSubscription-Timeout: %i seconds",
			mediaServer->_UDN.c_str(),
			mediaServer->_friendly_name.c_str(),
			mediaServer->m_modelDescription.c_str(),
			mediaServer->m_manufacturer.c_str(),
			mediaServer->m_deviceType.c_str(),
			mediaServer->_content_directory_event_url.c_str(),
			mediaServer->_content_directory_control_url.c_str(),
			(int)mediaServer->_i_content_directory_service_version,
			subscriptionId.c_str(),
			(int)mediaServer->_i_subscription_timeout)
		, mediaServer->_friendly_name, wxOK, this);
}


void SjUpnpDialog::OnDirDoubleClick(wxListEvent&)
{
	wxBusyCursor busy;

    SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
    if( mediaServer == NULL ) { return; } // nothing selected
    if( mediaServer != m_dirListFor ) { return; } // sth. went wrong

	// find out the clicked folder, clear directory entries
	wxString clickedId;
	{
		SjUpnpDirEntry* dirEntry = GetSelectedDirEntry();
		if( dirEntry == NULL ) { return; } // nothing selected
		clickedId = dirEntry->m_id;
		if( dirEntry != &m_parentDirEntry ) {
			m_parents.Add(m_currDir.getObjectID());
		}
		m_dirListCtrl->DeleteAllItems();
		m_currDir.Clear();
	}

	// load new directory entries
	m_currDir.setObjectID(clickedId);
    mediaServer->fetchContents(m_currDir);

    UpdateDirList();
}


void SjUpnpDialog::OnDirContextMenu(wxListEvent&)
{
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    bool sthSelected = GetSelectedDirEntry()!=NULL;

    SjMenu m(0);
    m.Append(IDC_DIRENTRYINFO, _("Info..."));
    m.Enable(IDC_DIRENTRYINFO, sthSelected);

    PopupMenu(&m, pt);
}


void SjUpnpDialog::OnDirEntryInfo(wxCommandEvent&)
{
	SjUpnpDirEntry* dirEntry = GetSelectedDirEntry();
	if( dirEntry == NULL ) { return; } // nothing selected

	wxString playtimeStr = "?";
	if( dirEntry->m_playtimeMs >= 0 ) {
		playtimeStr = SjTools::FormatMs(dirEntry->m_playtimeMs);
	}

	wxMessageBox(
		wxString::Format(
				"Name: %s\n\nDirectory: %i\n\nID: %s\n\nURL: %s\n\nPlaytime: %s",
				dirEntry->m_name.c_str(),
				(int)dirEntry->m_isDir,
				dirEntry->m_id.c_str(),
				dirEntry->m_url.c_str(),
				playtimeStr.c_str()
			)
			, dirEntry->m_name,
			wxOK, this);
}


BEGIN_EVENT_TABLE(SjUpnpDialog, SjDialog)
	EVT_LIST_ITEM_SELECTED    (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerClick       )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerContextMenu )
	EVT_MENU                  (IDC_MEDIASERVERINFO,      SjUpnpDialog::OnMediaServerInfo        )
	EVT_LIST_ITEM_ACTIVATED   (IDC_DIRLISTCTRL,          SjUpnpDialog::OnDirDoubleClick         )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_DIRLISTCTRL,          SjUpnpDialog::OnDirContextMenu         )
	EVT_MENU                  (IDC_DIRENTRYINFO,         SjUpnpDialog::OnDirEntryInfo           )
	EVT_SIZE                  (                          SjUpnpDialog::OnSize                   )
	EVT_MENU                  (MSG_UPDATEMEDIASERVERLIST,SjUpnpDialog::OnUpdateMediaServerList  )
	EVT_MENU                  (MSG_SCANDONE,             SjUpnpDialog::OnScanDone               )
END_EVENT_TABLE()


#endif // SJ_USE_UPNP


