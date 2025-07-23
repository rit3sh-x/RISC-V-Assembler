'use client';

import { Navbar } from '@/components/navbar';
import dynamic from 'next/dynamic';

const Landing = dynamic(
  () => import('@/components/home'),
  { ssr: false }
);

export default function Home() {
  return (
    <div className='w-full h-screen py-2 flex flex-col'>
      <Navbar />
      <div className="w-full h-full flex flex-col justify-center">
        <Landing />
      </div>
    </div>
  );
}