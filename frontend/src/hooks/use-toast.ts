"use client"

import * as React from "react"
import type {
  ToastActionElement,
  ToastProps,
} from "@/components/ui/toast"

const TOAST_LIMIT = 1
const TOAST_REMOVE_DELAY = 1000000

interface ToasterToast extends ToastProps {
  id: string
  description?: React.ReactNode
  action?: ToastActionElement
}

type Toast = Omit<ToasterToast, 'id'>

const actionTypes = {
  ADD_TOAST: "ADD_TOAST",
  UPDATE_TOAST: "UPDATE_TOAST",
  DISMISS_TOAST: "DISMISS_TOAST",
  REMOVE_TOAST: "REMOVE_TOAST",
} as const

type ActionType = typeof actionTypes

interface AddToastAction {
  type: ActionType["ADD_TOAST"]
  toast: ToasterToast
}

interface UpdateToastAction {
  type: ActionType["UPDATE_TOAST"]
  toast: Partial<ToasterToast>
}

interface DismissToastAction {
  type: ActionType["DISMISS_TOAST"]
  toastId?: string
}

interface RemoveToastAction {
  type: ActionType["REMOVE_TOAST"]
  toastId?: string
}

type Action = 
  | AddToastAction
  | UpdateToastAction
  | DismissToastAction
  | RemoveToastAction

interface State {
  toasts: ToasterToast[]
}

// Implementation
let count = 0

function genId(): string {
  count = (count + 1) % Number.MAX_SAFE_INTEGER
  return count.toString()
}

const toastTimeouts = new Map<string, NodeJS.Timeout>()

const addToRemoveQueue = (toastId: string): void => {
  if (toastTimeouts.has(toastId)) {
    return
  }

  const timeout = setTimeout(() => {
    toastTimeouts.delete(toastId)
    dispatch({
      type: "REMOVE_TOAST",
      toastId: toastId,
    })
  }, TOAST_REMOVE_DELAY)

  toastTimeouts.set(toastId, timeout)
}

export const reducer = (state: State, action: Action): State => {
  switch (action.type) {
    case "ADD_TOAST":
      return {
        ...state,
        toasts: [action.toast, ...state.toasts].slice(0, TOAST_LIMIT),
      }

    case "UPDATE_TOAST":
      return {
        ...state,
        toasts: state.toasts.map((t) =>
          t.id === action.toast.id ? { ...t, ...action.toast } : t
        ),
      }

    case "DISMISS_TOAST": {
      const { toastId } = action
      if (toastId) {
        addToRemoveQueue(toastId)
      } else {
        state.toasts.forEach((toast) => {
          addToRemoveQueue(toast.id)
        })
      }

      return {
        ...state,
        toasts: state.toasts.map((t) =>
          t.id === toastId || toastId === undefined
            ? { ...t, open: false }
            : t
        ),
      }
    }

    case "REMOVE_TOAST":
      if (action.toastId === undefined) {
        return { ...state, toasts: [] }
      }
      return {
        ...state,
        toasts: state.toasts.filter((t) => t.id !== action.toastId),
      }

    default:
      return state
  }
}

const listeners: Array<(state: State) => void> = []
let memoryState: State = { toasts: [] }

function dispatch(action: Action): void {
  memoryState = reducer(memoryState, action)
  listeners.forEach((listener) => listener(memoryState))
}

interface ToastReturn {
  id: string
  dismiss: () => void
  update: (props: Partial<ToasterToast>) => void
}

function toast(props: Toast): ToastReturn {
  const id = genId()

  const update = (props: Partial<ToasterToast>): void =>
    dispatch({
      type: "UPDATE_TOAST",
      toast: { ...props, id },
    })

  const dismiss = (): void => 
    dispatch({ type: "DISMISS_TOAST", toastId: id })

  dispatch({
    type: "ADD_TOAST",
    toast: {
      ...props,
      id,
      open: true,
      onOpenChange: (open: boolean) => {
        if (!open) dismiss()
      },
    },
  })

  return { id, dismiss, update }
}

interface UseToastReturn extends State {
  toast: (props: Toast) => ToastReturn
  dismiss: (toastId?: string) => void
}

function useToast(): UseToastReturn {
  const [state, setState] = React.useState<State>(memoryState)

  React.useEffect(() => {
    listeners.push(setState)
    return () => {
      const index = listeners.indexOf(setState)
      if (index > -1) {
        listeners.splice(index, 1)
      }
    }
  }, [state])

  return {
    ...state,
    toast,
    dismiss: (toastId?: string) => dispatch({ type: "DISMISS_TOAST", toastId }),
  }
}

export { useToast, toast }
export type { Toast, ToasterToast, UseToastReturn }