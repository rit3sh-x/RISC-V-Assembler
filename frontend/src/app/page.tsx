'use client';

import dynamic from 'next/dynamic';

const Landing = dynamic(
  () => import('@/components/Landing'),
  { ssr: false }
);

export default function Home() {
  return (
    <Landing />
  );
}