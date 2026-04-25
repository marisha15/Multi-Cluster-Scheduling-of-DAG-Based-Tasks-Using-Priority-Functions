#include <bits/stdc++.h>
using namespace std;

struct Task {
    int id, app_id, cluster_type;
    double duration, resource_req;

    double CP;
    double remWork;

    double ready_time;
    double arrival_time;  
    bool runnable;
    bool done;
    bool added;           

    vector<int> children;
    vector<int> parents;
};


// computing critical path (not used for FCFS decision)
double computeCP(int u, vector<Task> &tasks, vector<double> &dp) {
    if (dp[u] != -1) return dp[u];

    double max_child = 0;
    for (int v : tasks[u].children)
        max_child = max(max_child, computeCP(v, tasks, dp));

    return dp[u] = tasks[u].duration + max_child;
}


// get FCFS head (earliest ready_time, no resource constraint)
int getHeadTask(vector<Task> &tasks, int cluster_type) {
    int best = -1;
    double earliest = 1e18;

    for (auto &t : tasks) {
        if (!t.added) continue;
        if (!t.runnable || t.done) continue;
        if (t.cluster_type != cluster_type) continue;

        if (t.ready_time < earliest) {
            earliest = t.ready_time;
            best = t.id;
        }
    }
    return best;
}


// compute reservation time for head task
double computeReservationTime(int task_id,
                             vector<Task> &tasks,
                             vector<pair<double,int>> &running,
                             double available_resource) {

    double needed = tasks[task_id].resource_req;
    double curr = available_resource;

    if (curr >= needed) return 0;

    vector<pair<double,int>> temp = running;
    sort(temp.begin(), temp.end());

    for (auto &r : temp) {
        int tid = r.second;

        if (tasks[tid].cluster_type != tasks[task_id].cluster_type)
            continue;

        curr += tasks[tid].resource_req;

        if (curr >= needed)
            return r.first;
    }

    return 1e18;
}


// completing a task
void completeTask(int id, vector<Task> &tasks, double current_time,
                  unordered_map<int,double> &remWork) {

    tasks[id].done = true;
    tasks[id].runnable = false;

    remWork[tasks[id].app_id] -= tasks[id].duration;

    for (int child : tasks[id].children) {

        bool ready = true;
        for (int p : tasks[child].parents) {
            if (!tasks[p].done) {
                ready = false;
                break;
            }
        }

        if (ready && !tasks[child].done) {
            tasks[child].runnable = true;
            tasks[child].ready_time = current_time;
        }
    }
}


int main() {

    int n;
    cin >> n;   // number of tasks

    vector<Task> tasks(n);

    // read input
    // id app_id cluster_type duration resource arrival k parents...
    for (int i = 0; i < n; i++) {

        int id, app_id, cluster_type, k;
        double duration, resource_req, arrival_time;

        cin >> id >> app_id >> cluster_type >> duration >> resource_req >> arrival_time >> k;

        tasks[id].id = id;
        tasks[id].app_id = app_id;
        tasks[id].cluster_type = cluster_type;
        tasks[id].duration = duration;
        tasks[id].resource_req = resource_req;
        tasks[id].arrival_time = arrival_time;

        tasks[id].CP = 0;
        tasks[id].remWork = 0;
        tasks[id].ready_time = -1;
        tasks[id].runnable = false;
        tasks[id].done = false;
        tasks[id].added = false;

        tasks[id].parents.clear();
        tasks[id].children.clear();

        for (int j = 0; j < k; j++) {
            int p; cin >> p;
            tasks[id].parents.push_back(p);
        }
    }

    // build dag
    for (auto &t : tasks)
        for (int p : t.parents)
            tasks[p].children.push_back(t.id);

    double current_time = 0;

    double cpu_available = 4;
    double gpu_available = 2;

    vector<double> dp(n, -1);
    for (int i = 0; i < n; i++)
        tasks[i].CP = computeCP(i, tasks, dp);

    unordered_map<int,double> remWork;
    for (auto &t : tasks)
        remWork[t.app_id] += t.duration;

    vector<pair<double,int>> running;

    // main loop
    while (true) {

        // arrivals
        for (auto &t : tasks) {
            if (!t.added && t.arrival_time <= current_time) {
                t.added = true;

                if (t.parents.empty()) {
                    t.runnable = true;
                    t.ready_time = current_time;
                }
            }
        }

        // update remwork
        for (auto &t : tasks)
            t.remWork = remWork[t.app_id];

        // cpu scheduling
        while (true) {

            int head = getHeadTask(tasks, 0);
            double reservation = 1e18;

            if (head != -1 && tasks[head].resource_req > cpu_available) {
                reservation = computeReservationTime(head, tasks, running, cpu_available);
            }

            int chosen = -1;

            for (auto &t : tasks) {

                if (!t.added || !t.runnable || t.done) continue;
                if (t.cluster_type != 0) continue;
                if (t.resource_req > cpu_available) continue;

                if (t.id == head) {
                    chosen = t.id;
                    break;
                }

                double finish_time = current_time + t.duration;

                if (finish_time <= reservation) {
                    chosen = t.id;
                    break;
                }
            }

            if (chosen == -1) break;

            running.push_back({current_time + tasks[chosen].duration, chosen});
            cpu_available -= tasks[chosen].resource_req;
            tasks[chosen].runnable = false;
        }

        // gpu scheduling
        while (true) {

            int head = getHeadTask(tasks, 1);
            double reservation = 1e18;

            if (head != -1 && tasks[head].resource_req > gpu_available) {
                reservation = computeReservationTime(head, tasks, running, gpu_available);
            }

            int chosen = -1;

            for (auto &t : tasks) {

                if (!t.added || !t.runnable || t.done) continue;
                if (t.cluster_type != 1) continue;
                if (t.resource_req > gpu_available) continue;

                if (t.id == head) {
                    chosen = t.id;
                    break;
                }

                double finish_time = current_time + t.duration;

                if (finish_time <= reservation) {
                    chosen = t.id;
                    break;
                }
            }

            if (chosen == -1) break;

            running.push_back({current_time + tasks[chosen].duration, chosen});
            gpu_available -= tasks[chosen].resource_req;
            tasks[chosen].runnable = false;
        }

        // idle to jump to next arrival
        if (running.empty()) {
            double next_arrival = 1e18;

            for (auto &t : tasks)
                if (!t.added)
                    next_arrival = min(next_arrival, t.arrival_time);

            if (next_arrival == 1e18) break;

            current_time = next_arrival;
            continue;
        }

        // next event
        double next_completion = 1e18;
        for (auto &r : running)
            next_completion = min(next_completion, r.first);

        double next_arrival = 1e18;
        for (auto &t : tasks)
            if (!t.added)
                next_arrival = min(next_arrival, t.arrival_time);

        current_time = min(next_completion, next_arrival);

        // handle completions
        vector<pair<double,int>> new_running;

        for (auto &r : running) {
            if (r.first == current_time) {

                cout << "Completed task: " << r.second 
                     << " at time " << current_time << endl;

                if (tasks[r.second].cluster_type == 0)
                    cpu_available += tasks[r.second].resource_req;
                else
                    gpu_available += tasks[r.second].resource_req;

                completeTask(r.second, tasks, current_time, remWork);

            } else {
                new_running.push_back(r);
            }
        }

        running = new_running;
    }

    cout << "All tasks completed at time: " << current_time << endl;
}