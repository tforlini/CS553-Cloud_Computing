public class StaticProvisioning extends ProvisioningModule {
    private int count;

    /**
     * Build a static provisioning module that run a definite number of workers.
     * @param count number of worker instances to run.
     */
    public StaticProvisioning(int count) {
        super();
        this.count = count;
    }

    @Override
    protected void provisionWorkers() {
        for (int i = 0; i < count; i++) {
            addWorker();
        }
    }
}
