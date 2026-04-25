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


// computing critical path
double computeCP(int u, vector<Task> &tasks, vector<double> &dp) {
    if (dp[u] != -1) return dp[u];

    double max_child = 0;
    for (int v : tasks[u].children) {
        max_child = max(max_child, computeCP(v, tasks, dp));
    }

    return dp[u] = tasks[u].duration + max_child;
}


// priority function
double computePriority(Task &t, double current_time, double fit, double depPenalty) {
    double waiting = current_time - t.ready_time;

    double w1 = 1, w2 = 0.4, w3 = 0.2, alpha = 0.08, w4 = 1;

    return w1 * t.CP
         + w2 * fit
         - w3 * t.remWork
         + alpha * waiting
         - w4 * depPenalty;
}


// selecting next task 
int selectNextTask(vector<Task> &tasks, double current_time, double available_resource, int cluster_type) {
    int best_task = -1;
    double best_score = -1e18;

    for (auto &t : tasks) {

        if (!t.added) continue;                
        if (!t.runnable || t.done) continue;
        if (t.cluster_type != cluster_type) continue;

        double fit = t.resource_req / available_resource;

        double depPenalty = 0;
        for (int child : t.children) {
            for (int p : tasks[child].parents) {
                if (p != t.id && !tasks[p].done) {
                    depPenalty = max(depPenalty, tasks[p].duration);
                }
            }
        }

        double score = computePriority(t, current_time, fit, depPenalty);

        if (t.resource_req <= available_resource && score > best_score) {
            best_score = score;
            best_task = t.id;
        }
    }

    return best_task;
}


// completing a task
void completeTask(int id, vector<Task> &tasks, double current_time,
                  unordered_map<int, double> &remWork) {

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

    // read tasks
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
            int p;
            cin >> p;
            tasks[id].parents.push_back(p);
        }
    }

    // build children
    for (auto &t : tasks) {
        for (int p : t.parents) {
            tasks[p].children.push_back(t.id);
        }
    }

    double current_time = 0;

    double cpu_available = 4;
    double gpu_available = 2;

    vector<double> dp(tasks.size(), -1);
    for (int i = 0; i < tasks.size(); i++) {
        tasks[i].CP = computeCP(i, tasks, dp);
    }

    unordered_map<int, double> remWork;
    for (auto &t : tasks) {
        remWork[t.app_id] += t.duration;
    }

    vector<pair<double,int>> running;

    while (true) {

        // add arrived tasks
        for (auto &t : tasks) {
            if (!t.added && t.arrival_time <= current_time) {
                t.added = true;

                if (t.parents.empty()) {
                    t.runnable = true;
                    t.ready_time = current_time;
                }
            }
        }

        // update remWork
        for (auto &t : tasks) {
            t.remWork = remWork[t.app_id];
        }

        // schedule CPU
        while (true) {
            int next_cpu = selectNextTask(tasks, current_time, cpu_available, 0);
            if (next_cpu == -1) break;
            if (tasks[next_cpu].resource_req > cpu_available) break;

            running.push_back({current_time + tasks[next_cpu].duration, next_cpu});
            cpu_available -= tasks[next_cpu].resource_req;
            tasks[next_cpu].runnable = false;
        }

        // schedule GPU
        while (true) {
            int next_gpu = selectNextTask(tasks, current_time, gpu_available, 1);
            if (next_gpu == -1) break;
            if (tasks[next_gpu].resource_req > gpu_available) break;

            running.push_back({current_time + tasks[next_gpu].duration, next_gpu});
            gpu_available -= tasks[next_gpu].resource_req;
            tasks[next_gpu].runnable = false;
        }

        if (running.empty()) {
            double next_arrival = 1e18;
            for (auto &t : tasks) {
                if (!t.added)
                    next_arrival = min(next_arrival, t.arrival_time);
            }
            if (next_arrival == 1e18) break;
            current_time = next_arrival;
            continue;
        }

        double next_time = 1e18;
        for (auto &r : running)
            next_time = min(next_time, r.first);

        double next_arrival = 1e18;
        for (auto &t : tasks)
            if (!t.added)
                next_arrival = min(next_arrival, t.arrival_time);

        current_time = min(next_time, next_arrival);

        vector<pair<double,int>> new_running;

        for (auto &r : running) {
            if (r.first == current_time) {

                cout << "Completed task: " << r.second << " at time " << current_time << endl;

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

    return 0;
}