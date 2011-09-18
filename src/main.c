/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>
#include <stdio.h>
#include "../res/dialog.h"
#include "http.h"
#include "net.h"
#include <commctrl.h>

HWND hwnd_status;
HWND hwnd_settings;
HWND itm_test;
HWND itm_cancel;

HWND itm_port_int;
HWND itm_port_ext;

int port_int;
int port_ext;

void CenterWindow(HWND hWnd)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);
    SetWindowPos(hWnd, NULL, GetSystemMetrics(SM_CXSCREEN) / 2 - (rc.right - rc.left) / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - (rc.bottom - rc.top) / 2, rc.right - rc.left, rc.bottom - rc.top, 0);
}

DWORD WINAPI cncnet_tester(int ctx)
{
    HWND itm_status = GetDlgItem(hwnd_status, ITM_STATUS);
    char buf[512] = { 0 };
    char url[512];
    struct sockaddr_in from;
    fd_set read_fds;
    struct timeval tv;
    int ret;
    int port_int;
    int port;

    CenterWindow(hwnd_status);
    ShowWindow(hwnd_status, SW_SHOW);
    SetForegroundWindow(hwnd_status);

    net_init();
    http_init();

    GetWindowText(itm_port_int, buf, sizeof(buf));
    port_int = atoi(buf);
    GetWindowText(itm_port_ext, buf, sizeof(buf));
    port = atoi(buf);

    net_bind("0.0.0.0", port_int);

    if (!net_socket)
    {
        SetWindowText(itm_status, "Error creating socket :-(");
        return 0;
    }

    sprintf(buf, "Requesting for UDP packet to port %d...", port);
    SetWindowText(itm_status, buf);

    sprintf(url, "http://hifi.iki.fi/cncnet/ping.php?port=%d", port);
    if (http_download_mem(url, buf, sizeof(buf)))
    {
        SetWindowText(itm_status, "Waiting for response...");

        tv.tv_sec = 3;
        tv.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(net_socket, &read_fds);

        select(net_socket+1, &read_fds, NULL, NULL, &tv);

        ShowWindow(hwnd_status, SW_HIDE);

        if(FD_ISSET(net_socket, &read_fds))
        {
            ret = net_recv(&from);

            if (ret == 4)
            {
                sprintf(buf, "Congratulations! UDP port %d is correctly forwarded to your local port %d.\n", port, port_int);
                MessageBox(hwnd_status, buf, "CnCNet Tester - Success!", MB_OK|MB_ICONINFORMATION);
            }
            else
            {
                sprintf(buf, "Invalid UDP packet, you running something else on internal port %d?", port_int);
                MessageBox(hwnd_status, buf, "CnCNet Tester - Error", MB_OK|MB_ICONEXCLAMATION);
            }
        }
        else
        {
            sprintf(buf, "Timed out. UDP port %d is not forwarded correctly :-(", port);
            MessageBox(hwnd_status, buf, "CnCNet Tester - Error", MB_OK|MB_ICONSTOP);
        }
    }
    else
    {
        MessageBox(hwnd_status, "Testing server is down, sorry :-(", "CnCNet Tester - Error", MB_OK|MB_ICONSTOP);
    }

    http_release();
    net_free();

    PostMessage(hwnd_status, WM_COMMAND, 0, 0);

    return 0;
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        return TRUE;
    }

    if (uMsg == WM_COMMAND && (lParam == 0 || lParam == (LPARAM)itm_cancel))
    {
        PostQuitMessage(0);
    }

    if (uMsg == WM_COMMAND && lParam == (LPARAM)itm_test)
    {
        ShowWindow(hwnd, SW_HIDE);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cncnet_tester, 0, 0, NULL);
    }

    return FALSE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    BOOL ret;
    MSG msg;
    hwnd_status = CreateDialog(NULL, MAKEINTRESOURCE(DLG_STATUS), NULL, DialogProc);
    hwnd_settings = CreateDialog(NULL, MAKEINTRESOURCE(DLG_SETTINGS), NULL, DialogProc);
    itm_port_int = GetDlgItem(hwnd_settings, ITM_PORT_INT);
    itm_port_ext = GetDlgItem(hwnd_settings, ITM_PORT_EXT);
    itm_test = GetDlgItem(hwnd_settings, ITM_TEST);
    itm_cancel = GetDlgItem(hwnd_settings, ITM_CANCEL);

    SendMessage(hwnd_status, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(hInstance, "small"));
    SendMessage(hwnd_status, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(hInstance, "large"));
    SendMessage(hwnd_settings, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(hInstance, "small"));
    SendMessage(hwnd_settings, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(hInstance, "large"));

    /* set initial focus */
    SendMessage(hwnd_settings, WM_NEXTDLGCTL, (WPARAM)itm_test, TRUE);

    SetWindowText(itm_port_int, "8054");
    SetWindowText(itm_port_ext, "8054");

    CenterWindow(hwnd_settings);
    ShowWindow(hwnd_settings, SW_SHOW);
    SetForegroundWindow(hwnd_settings);

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) 
    { 
        if (ret == -1)
        {
            break;
        }
        else if ((!IsWindow(hwnd_status) || !IsDialogMessage(hwnd_status, &msg)) && (!IsWindow(hwnd_settings) || !IsDialogMessage(hwnd_settings, &msg)))
        { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
    } 

    return 0;
}
