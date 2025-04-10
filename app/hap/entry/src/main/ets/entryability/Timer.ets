// 管理多个定时器状态的Map
interface TimerInterface {
    timerId: number | null;
    timerRunning: boolean;
}

const timers = new Map<number, TimerInterface>();

export function startTimer(
    timerKey: number,
    callback: () => void,
    interval: number
) {
    if (!timers.has(timerKey)) {
        timers.set(timerKey, { timerId: null, timerRunning: false });
    }

    const timerState = timers.get(timerKey)!;

    if (!timerState.timerRunning) {
        timerState.timerId = setInterval(callback, interval);
        timerState.timerRunning = true;
    }
}

export function pauseTimer(timerKey: number) {
    if (timers.has(timerKey)) {
        const timerState = timers.get(timerKey)!;

        if (timerState.timerRunning && timerState.timerId !== null) {
            clearInterval(timerState.timerId);
            timerState.timerId = null;
            timerState.timerRunning = false;
        }
    }
}

export function resumeTimer(
    timerKey: number,
    callback: () => void,
    interval: number
) {
    if (timers.has(timerKey)) {
        const timerState = timers.get(timerKey)!;

        if (!timerState.timerRunning) {
            timerState.timerId = setInterval(callback, interval);
            timerState.timerRunning = true;
        }
    }
}

export function stopTimer(timerKey: number) {
    if (timers.has(timerKey)) {
        const timerState = timers.get(timerKey)!;

        if (timerState.timerId !== null) {
            clearInterval(timerState.timerId);
        }

        timers.delete(timerKey); // 移除该定时器的状态
    }
}
