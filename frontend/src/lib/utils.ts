import { clsx, type ClassValue } from "clsx"
import { twMerge } from "tailwind-merge"

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

const toSignedDecimal = (value: number, bits = 32) => {
  const maxPositive = Math.pow(2, bits - 1) - 1;
  if (value > maxPositive) {
    return value - Math.pow(2, bits);
  }
  return value;
};

export const byteToSignedDecimal = (value: number) => toSignedDecimal(value, 8);
export const wordToSignedDecimal = (value: number) => toSignedDecimal(value, 32);