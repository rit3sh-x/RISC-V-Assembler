import type { Metadata } from "next";
import { Toaster } from "sonner";
import "./globals.css";

export const metadata: Metadata = {
  title: "RISC-V Simulator",
  description: "Simulate RISC-V assembly code in your browser",
  icons: {
    icon: "/logo.svg",
  },
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>
        {children}
        <Toaster 
          position="bottom-right" 
          richColors 
          pauseWhenPageIsHidden 
          closeButton 
          toastOptions={{ style: { zIndex: 9999 } }}
          expand={true}
          visibleToasts={5}
        />
      </body>
    </html>
  );
}