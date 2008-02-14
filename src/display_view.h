//-----------------------------------------------------------------------------
//
//	MONOGRAM GraphStudio
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

namespace GraphStudio
{

	//-------------------------------------------------------------------------
	//
	//	DisplayView class
	//
	//-------------------------------------------------------------------------
	class DisplayView : public CScrollView, public GraphCallback
	{
	protected:
		DECLARE_DYNCREATE(DisplayView)
		DECLARE_MESSAGE_MAP()

	public:

		RenderParameters	render_params;

		// graph currently displayed
		DisplayGraph		graph;

		// double buffered view
		CBitmap				backbuffer;
		CDC					memDC;
		int					back_width, back_height;

		CPoint				start_drag_point;
		CPoint				end_drag_point;

		// creating new connection
		CPoint				new_connection_start;
		CPoint				new_connection_end;

		enum {
			DRAG_GROUP = 0,
			DRAG_CONNECTION = 1,
			DRAG_SELECTION = 2
		};
		int					drag_mode;

		// helpers for rightclick menu
		Filter				*current_filter;
		Pin					*current_pin;

	public:
		DisplayView();
		~DisplayView();

		BOOL OnEraseBkgnd(CDC* pDC);
		virtual void OnDraw(CDC *pDC);
		void OnSize(UINT nType, int cx, int cy);

		void OnMouseMove(UINT nFlags, CPoint point);
		void OnLButtonDown(UINT nFlags, CPoint point);
		void OnLButtonUp(UINT nFlags, CPoint point);
		void OnRButtonDown(UINT nFlags, CPoint point);
		void OnLButtonDblClk(UINT nFlags, CPoint point);

		void OnRenderPin();
		void OnPropertyPage();

		void MakeScreenshot();

		// scrolling aid
		void UpdateScrolling();
		void RepaintBackbuffer();

		// to be overriden
		virtual void OnDisplayPropertyPage(IUnknown *object, IUnknown *filter, CString title);
		virtual void OnFilterRemoved(DisplayGraph *sender, Filter *filter);
	};


};


