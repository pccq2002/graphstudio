//-----------------------------------------------------------------------------
//
//	MONOGRAM GraphStudio
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "PropertyForm.h"


//-----------------------------------------------------------------------------
//
//	CPropertyForm class
//
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CPropertyForm, CDialog)
BEGIN_MESSAGE_MAP(CPropertyForm, CDialog)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_PAGES, &CPropertyForm::OnTabSelected)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CPropertyForm::OnBnClickedButtonApply)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
//
//	CPropertyForm class
//
//-----------------------------------------------------------------------------

CPropertyForm::CPropertyForm(CWnd *pParent) : 
	CDialog(CPropertyForm::IDD, pParent)
{
	container = NULL;
	object = NULL;
	filter = NULL;
}

CPropertyForm::~CPropertyForm()
{
	if (object) object->Release(); object = NULL;
}

void CPropertyForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_PAGES, tabs);
	DDX_Control(pDX, IDC_BUTTON_APPLY, button_apply);
	DDX_Control(pDX, IDOK, button_ok);
	DDX_Control(pDX, IDCANCEL, button_close);
}

void CPropertyForm::OnClose()
{
	// report that we're being closed
	view->ClosePropertyPage(filter);
}

void CPropertyForm::OnOK()
{
	OnBnClickedButtonApply();
	OnClose();
}

void CPropertyForm::OnCancel()
{
	OnClose();
}

void CPropertyForm::OnDestroy()
{
	// destroy our pages
	if (container) {
		container->Clear();
		delete container;
	}
	if (object) object->Release(); object = NULL;
	if (filter) filter->Release(); filter = NULL;

	__super::OnDestroy();
}

void CPropertyForm::ResizeToFit(CSize size)
{
	// compute alignment helpers
	CRect	rc_client;
	CRect	rc_display;

	GetWindowRect(&rc_client);
	tabs.GetClientRect(&rc_display);
	tabs.AdjustRect(FALSE, &rc_display);

	int	dx = rc_client.Width() - rc_display.Width();
	int	dy = rc_client.Height()- rc_display.Height();

	// resize the parent window
	SetWindowPos(NULL, 0, 0, (size.cx + dx), (size.cy + dy), SWP_NOMOVE);
}

void CPropertyForm::OnSize(UINT nType, int cx, int cy)
{
	if (IsWindow(tabs)) {
		tabs.SetWindowPos(NULL, tab_x, tab_y, cx-tab_cx, cy-tab_cy, SWP_SHOWWINDOW);

		button_ok.SetWindowPos(NULL, cx-bok_cx, cy-button_bottom_offset, 0, 0, SWP_NOSIZE);
		button_close.SetWindowPos(NULL, cx-bcancel_cx, cy-button_bottom_offset, 0, 0, SWP_NOSIZE);
		button_apply.SetWindowPos(NULL, cx-bapply_cx, cy-button_bottom_offset, 0, 0, SWP_NOSIZE);
	}
}

int CPropertyForm::DisplayPages(IUnknown *obj, IUnknown *filt, CString title, CGraphView *view)
{
	// create a new window
	BOOL bret = Create(IDD_DIALOG_PROPERTYPAGE);
	if (!bret) return -1;
	SetWindowText(title);

	// we need to set WS_EX_CONTROLPARENT for the tabs
	tabs.ModifyStyleEx(0, WS_EX_CONTROLPARENT);

	if (object) object->Release(); object = NULL;
	if (filter) filter->Release(); filter = NULL;

	object = obj;
	object->AddRef();
	this->view = view;

	if (filt) {
		filter = filt;
		filter->AddRef();
	}

	// compute alignment helpers
	CRect	rc_client;
	CRect	rc_display;

	GetWindowRect(&rc_client);

	CPoint	p1(0,0), p2(0,0);
	ClientToScreen(&p1);
	tabs.ClientToScreen(&p2);
	GetClientRect(&rc_client);
	tabs.GetWindowRect(&rc_display);
	tab_x = (p2.x-p1.x);		tab_y = (p2.y-p1.y);
	tab_cx = rc_client.Width() - rc_display.Width();
	tab_cy = rc_client.Height() - rc_display.Height();

	p2 = CPoint(0,0);
	button_ok.ClientToScreen(&p2);
	button_bottom_offset = p1.y + rc_client.Height() - p2.y;
	bok_cx = p1.x + rc_client.Width() - p2.x;

	p2 = CPoint(0,0);
	button_close.ClientToScreen(&p2);
	bcancel_cx = p1.x + rc_client.Width() - p2.x;

	p2 = CPoint(0,0);
	button_apply.ClientToScreen(&p2);
	bapply_cx = p1.x + rc_client.Width() - p2.x;


	// let's create a new container
	container = new CPageContainer(this);
	container->NonDelegatingAddRef();			// we may be exposing this object so make sure it won't go away

	int ret = AnalyzeObject(object);
	if (ret < 0) return -1;

	if (container->pages.GetCount() > 0) {
		container->ActivatePage(0);
	}

	// show page
	ShowWindow(SW_SHOW);
	return ret;
}

int CPropertyForm::AnalyzeObject(IUnknown *obj)
{
	CComPtr<ISpecifyPropertyPages>	specify;
	HRESULT							hr;

	CComPtr<IBaseFilter>			filter;
	hr = obj->QueryInterface(IID_IBaseFilter, (void**)&filter);
	if (FAILED(hr)) filter = NULL;

	CLSID		clsid;
	if (filter) {
		filter->GetClassID(&clsid);
	}

	bool		can_specify_pp = true;

	if (view->graph.is_remote) {

		// some filters don't like showing property page via remote connection
		if (clsid == CLSID_DSoundRender || clsid == CLSID_AudioRender) {
			can_specify_pp = false;
		}

	}

	if (can_specify_pp) {
		hr = obj->QueryInterface(IID_ISpecifyPropertyPages, (void**)&specify);
		if (SUCCEEDED(hr)) {
			CAUUID	pagelist;
			hr = specify->GetPages(&pagelist);
			if (SUCCEEDED(hr)) {

				// now create all pages
				for (int i=0; i<pagelist.cElems; i++) {
					CComPtr<IPropertyPage>	page;
					hr = CoCreateInstance(pagelist.pElems[i], NULL, CLSCTX_INPROC_SERVER, IID_IPropertyPage, (void**)&page);
					if (SUCCEEDED(hr)) {
						// assign the object
						hr = page->SetObjects(1, &obj);
						if (SUCCEEDED(hr)) {
							// and add the page to our container
							container->AddPage(page);
						}
					}

					page = NULL;
				}

				// free used memory
				if (pagelist.pElems) CoTaskMemFree(pagelist.pElems);
			}
		}
		specify = NULL;
	}
	
	filter = NULL;
	return 0;
}

void CPropertyForm::OnTabSelected(NMHDR *pNMHDR, LRESULT *pResult)
{
	int i = tabs.GetCurSel();
	if (container) container->ActivatePage(i);
	*pResult = 0;
}


void CPropertyForm::OnBnClickedButtonApply()
{
	if (container->current != -1) {
		CPageSite *site = container->pages[container->current];
		if (site->page) {
			site->page->Apply();
		}
	}
	button_apply.EnableWindow(FALSE);
}


//-----------------------------------------------------------------------------
//
//	CPageContainer class
//
//-----------------------------------------------------------------------------

CPageContainer::CPageContainer(CPropertyForm *parent) :
	CUnknown(NAME("Page Container"), NULL),
	form(parent)
{
	// we don't have any pages yet
	pages.RemoveAll();
	current = -1;
}

CPageContainer::~CPageContainer()
{
	Clear();
}

void CPageContainer::Clear()
{
	if (current != -1) DeactivatePage(current);

	// release all pages
	for (int i=0; i<pages.GetCount(); i++) {
		CPageSite	*site = pages[i];

		// make it go away
		site->CloseSite();
		delete site;
	}
	pages.RemoveAll();
	current = -1;

	// kick all pages in the tab control
	form->tabs.DeleteAllItems();
}

void CPageContainer::ActivatePage(int i)
{
	if (i == current) return ;

	CPageSite	*site = pages[i];

	if (current != -1) DeactivatePage(current);
	current = i;

	// select specified tab
	form->tabs.SetCurSel(i);

	// resize parent
	form->ResizeToFit(site->size);

	// now display the page
	CRect	rc_client;
	form->tabs.GetClientRect(&rc_client);
	form->tabs.AdjustRect(FALSE, &rc_client);

	// show the page
	site->Activate(form->tabs, rc_client);

	form->Invalidate();
}

void CPageContainer::DeactivatePage(int i)
{
	CPageSite	*site = pages[i];
	site->Deactivate();
	current = -1;
}

int CPageContainer::AddPage(IPropertyPage *page)
{
	// get some info first...
	PROPPAGEINFO	info;
	memset(&info, 0, sizeof(info));
	info.cb = sizeof(info);

	HRESULT	hr = page->GetPageInfo(&info);
	if (FAILED(hr)) return -1;

	CString	title = CString(info.pszTitle);
	CSize	size = info.size;

	// release buffers...
	if (info.pszTitle) CoTaskMemFree(info.pszTitle);
	if (info.pszDocString) CoTaskMemFree(info.pszDocString);
	if (info.pszHelpFile) CoTaskMemFree(info.pszHelpFile);

	// create site object for this page
	CPageSite	*site = new CPageSite(NULL, this);
	site->NonDelegatingAddRef();					// keep an extra reference
	site->AssignPage(page);
	site->title = title;
	site->size = size;

	// insert into tab control
	form->tabs.InsertItem(pages.GetCount(),title); 

	// we're done
	pages.Add(site);

	return 0;
}

//-----------------------------------------------------------------------------
//
//	CPageSite class
//
//-----------------------------------------------------------------------------

CPageSite::CPageSite(LPUNKNOWN pUnk, CPageContainer *container) :
	CUnknown(NAME("Page Site"), pUnk),
	parent(container),
	title(_T(""))
{
	page = NULL;
	size.cx = 0;
	size.cy = 0;
	active = false;
}

CPageSite::~CPageSite()
{
	page = NULL;
}

int CPageSite::CloseSite()
{
	if (page) {
		if (active) Deactivate();
		page = NULL;
	}
	return 0;
}

int CPageSite::AssignPage(IPropertyPage *page)
{
	// keep a reference
	page->QueryInterface(IID_IPropertyPage, (void**)&this->page);

	// pair the site object with page
	page->SetPageSite(this);
	return 0;
}

STDMETHODIMP CPageSite::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IPropertyPageSite) {
		return GetInterface((IPropertyPageSite*)this, ppv);
	} else
		return __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CPageSite::OnStatusChange(DWORD dwFlags)
{
	if (dwFlags & PROPPAGESTATUS_DIRTY) {
		parent->form->button_apply.EnableWindow(TRUE);
	}
	return NOERROR;
}

STDMETHODIMP CPageSite::GetLocaleID(LCID *pLocaleID)
{
	return E_FAIL;
}

STDMETHODIMP CPageSite::GetPageContainer(IUnknown **ppUnk)
{
	return parent->NonDelegatingQueryInterface(IID_IUnknown, (void**)ppUnk);
}

STDMETHODIMP CPageSite::TranslateAccelerator(MSG *pMsg)
{
	return NOERROR;
}

int CPageSite::Deactivate()
{
	if (page) {
		page->Deactivate();
		active = false;
	}
	return 0;
}

int CPageSite::Activate(HWND owner, CRect &rc)
{
	parent->form->button_apply.EnableWindow(FALSE);
	if (page) {
		HRESULT hr = page->Activate(owner, &rc, FALSE);
		if (FAILED(hr)) return -1;
		active = true;
	}
	return 0;
}


